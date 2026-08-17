#pragma once

#include "Digest.h"
#include "Export.h"
#include <QObject>
#include <QHash>
#include <QSet>
#include <QStringList>

namespace slang_qrhi {

Q_NAMESPACE

enum class NodeKind : quint8 {
    Source,
    Module,
    EntryPoint,
    UiSchema,
    ParameterLayout,
    ParameterValues,
    Resource,
    BindingLayout,
    RenderState,
    Pipeline,
};
Q_ENUM_NS(NodeKind)

enum DirtyFlag : quint32 {
    Clean = 0,
    UiDirty = 1u << 0,
    UniformDirty = 1u << 1,
    ResourceDirty = 1u << 2,
    BindingDirty = 1u << 3,
    ShaderDirty = 1u << 4,
    PipelineDirty = 1u << 5,
};
Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)
Q_FLAG_NS(DirtyFlags)

using NodeId = quint64;

struct GraphNode {
    NodeId id = 0;
    QString key;
    NodeKind kind = NodeKind::Source;
    QByteArray payload;
    Digest localDigest;
    Digest merkleDigest;
    QList<NodeId> dependencies;
    QSet<NodeId> dependents;
    DirtyFlags dirty;
};

class SLANG_QRHI_EXPORT DependencyGraph final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY graphChanged)

public:
    explicit DependencyGraph(QObject* parent = nullptr);

    Q_INVOKABLE NodeId ensureNode(const QString& key, NodeKind kind);
    Q_INVOKABLE bool contains(const QString& key) const;
    Q_INVOKABLE NodeId nodeId(const QString& key) const;
    Q_INVOKABLE QString digestHex(NodeId id) const;
    Q_INVOKABLE quint32 dirtyFlags(NodeId id) const;
    Q_INVOKABLE QStringList dependencies(NodeId id) const;
    Q_INVOKABLE QByteArray payload(NodeId id) const;
    Q_INVOKABLE int nodeCount() const { return m_nodes.size(); }

    bool setPayload(NodeId id, QByteArray payload);
    bool setDependencies(NodeId id, const QList<NodeId>& deps);
    void markClean(NodeId id,
        DirtyFlags flags = DirtyFlags(
            UiDirty | UniformDirty | ResourceDirty | BindingDirty | ShaderDirty | PipelineDirty));
    void markAllClean();

signals:
    void nodeChanged(quint64 id, quint32 dirtyFlags);
    void graphChanged();

private:
    const GraphNode* node(NodeId id) const;
    GraphNode* node(NodeId id);
    DirtyFlags intrinsicDirty(NodeKind kind) const;
    Digest recompute(NodeId id, QSet<NodeId>& active);
    void recomputeAndPropagate(NodeId start);
    void propagateDirty(NodeId id, DirtyFlags flags, QSet<NodeId>& visited);

    NodeId m_nextId = 1;
    QHash<NodeId, GraphNode> m_nodes;
    QHash<QString, NodeId> m_byKey;
};

} // namespace slang_qrhi

Q_DECLARE_OPERATORS_FOR_FLAGS(slang_qrhi::DirtyFlags)
