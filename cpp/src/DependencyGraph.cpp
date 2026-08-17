#include <slang_qrhi/DependencyGraph.h>
#include <algorithm>
#include <stdexcept>

namespace slang_qrhi {

DependencyGraph::DependencyGraph(QObject* parent)
    : QObject(parent)
{
}

NodeId DependencyGraph::ensureNode(const QString& key, NodeKind kind)
{
    if (const auto it = m_byKey.constFind(key); it != m_byKey.cend()) {
        auto& n = m_nodes[*it];
        if (n.kind != kind) {
            n.kind = kind;
            recomputeAndPropagate(n.id);
        }
        return n.id;
    }
    GraphNode n;
    n.id = m_nextId++;
    n.key = key;
    n.kind = kind;
    n.localDigest = Digest::hash({});
    n.merkleDigest = Digest::combine(QByteArrayView(key.toUtf8()), {}, {});
    m_nodes.insert(n.id, n);
    m_byKey.insert(key, n.id);
    emit graphChanged();
    return n.id;
}

bool DependencyGraph::contains(const QString& key) const
{
    return m_byKey.contains(key);
}
NodeId DependencyGraph::nodeId(const QString& key) const
{
    return m_byKey.value(key, 0);
}

QString DependencyGraph::digestHex(NodeId id) const
{
    if (const auto* n = node(id))
        return n->merkleDigest.hex();
    return {};
}

quint32 DependencyGraph::dirtyFlags(NodeId id) const
{
    if (const auto* n = node(id))
        return quint32(n->dirty.toInt());
    return 0;
}

QStringList DependencyGraph::dependencies(NodeId id) const
{
    QStringList out;
    if (const auto* n = node(id)) {
        for (NodeId d : n->dependencies)
            if (const auto* dep = node(d))
                out << dep->key;
    }
    return out;
}

QByteArray DependencyGraph::payload(NodeId id) const
{
    if (const auto* n = node(id))
        return n->payload;
    return {};
}

const GraphNode* DependencyGraph::node(NodeId id) const
{
    const auto it = m_nodes.constFind(id);
    return it == m_nodes.cend() ? nullptr : &it.value();
}
GraphNode* DependencyGraph::node(NodeId id)
{
    auto it = m_nodes.find(id);
    return it == m_nodes.end() ? nullptr : &it.value();
}

DirtyFlags DependencyGraph::intrinsicDirty(NodeKind kind) const
{
    switch (kind) {
    case NodeKind::UiSchema:
        return UiDirty;
    case NodeKind::ParameterValues:
        return UniformDirty;
    case NodeKind::Resource:
        return ResourceDirty;
    case NodeKind::BindingLayout:
        return BindingDirty | PipelineDirty;
    case NodeKind::Source:
    case NodeKind::Module:
    case NodeKind::EntryPoint:
        return ShaderDirty | PipelineDirty;
    case NodeKind::ParameterLayout:
        return UiDirty | UniformDirty | BindingDirty | PipelineDirty;
    case NodeKind::RenderState:
    case NodeKind::Pipeline:
        return PipelineDirty;
    }
    return PipelineDirty;
}

bool DependencyGraph::setPayload(NodeId id, QByteArray payload)
{
    auto* n = node(id);
    if (!n || n->payload == payload)
        return false;
    n->payload = std::move(payload);
    n->localDigest = Digest::hash(n->payload);
    recomputeAndPropagate(id);
    return true;
}

bool DependencyGraph::setDependencies(NodeId id, const QList<NodeId>& deps)
{
    auto* n = node(id);
    if (!n)
        return false;
    QList<NodeId> canonical = deps;
    std::sort(canonical.begin(), canonical.end(), [&](NodeId a, NodeId b) {
        const auto* na = node(a);
        const auto* nb = node(b);
        return na && nb ? na->key < nb->key : a < b;
    });
    canonical.erase(std::unique(canonical.begin(), canonical.end()), canonical.end());
    if (canonical == n->dependencies)
        return false;

    for (NodeId old : n->dependencies)
        if (auto* dep = node(old))
            dep->dependents.remove(id);
    n->dependencies = canonical;
    for (NodeId depId : canonical) {
        if (depId == id)
            throw std::logic_error("DependencyGraph self-cycle");
        if (auto* dep = node(depId))
            dep->dependents.insert(id);
    }
    recomputeAndPropagate(id);
    return true;
}

Digest DependencyGraph::recompute(NodeId id, QSet<NodeId>& active)
{
    auto* n = node(id);
    if (!n)
        return {};
    if (active.contains(id))
        throw std::logic_error("DependencyGraph cycle detected");
    active.insert(id);
    QList<QPair<QByteArray, Digest>> deps;
    deps.reserve(n->dependencies.size());
    for (NodeId depId : n->dependencies) {
        const auto* dep = node(depId);
        if (!dep)
            continue;
        deps.push_back({ dep->key.toUtf8(), dep->merkleDigest });
    }
    active.remove(id);
    const auto domain = QByteArray::number(int(n->kind)) + ':' + n->key.toUtf8();
    return Digest::combine(domain, n->payload, deps);
}

void DependencyGraph::propagateDirty(NodeId id, DirtyFlags flags, QSet<NodeId>& visited)
{
    if (visited.contains(id))
        return;
    visited.insert(id);
    auto* n = node(id);
    if (!n)
        return;
    n->dirty |= flags;
    emit nodeChanged(id, quint32(n->dirty.toInt()));
    for (NodeId parentId : n->dependents) {
        auto* parent = node(parentId);
        if (!parent)
            continue;
        const DirtyFlags next = flags | intrinsicDirty(parent->kind);
        propagateDirty(parentId, next, visited);
    }
}

void DependencyGraph::recomputeAndPropagate(NodeId start)
{
    auto* n = node(start);
    if (!n)
        return;
    const Digest old = n->merkleDigest;
    QSet<NodeId> active;
    n->merkleDigest = recompute(start, active);
    if (old == n->merkleDigest)
        return;

    QSet<NodeId> visited;
    propagateDirty(start, intrinsicDirty(n->kind), visited);

    // Recompute only downstream nodes whose inputs changed.
    QList<NodeId> queue = n->dependents.values();
    QSet<NodeId> queued;
    for (NodeId q : queue)
        queued.insert(q);
    while (!queue.isEmpty()) {
        const NodeId id = queue.takeFirst();
        queued.remove(id);
        auto* cur = node(id);
        if (!cur)
            continue;
        const Digest before = cur->merkleDigest;
        QSet<NodeId> recursion;
        cur->merkleDigest = recompute(id, recursion);
        if (before != cur->merkleDigest) {
            for (NodeId p : cur->dependents)
                if (!queued.contains(p)) {
                    queue.push_back(p);
                    queued.insert(p);
                }
        }
    }
    emit graphChanged();
}

void DependencyGraph::markClean(NodeId id, DirtyFlags flags)
{
    if (auto* n = node(id))
        n->dirty &= ~flags;
}

void DependencyGraph::markAllClean()
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
        it->dirty = {};
}

} // namespace slang_qrhi
