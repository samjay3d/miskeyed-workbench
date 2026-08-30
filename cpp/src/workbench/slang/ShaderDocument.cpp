#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace miskeyed::workbench::slang_rhi {

const CompiledEntryPoint* ShaderDocument::findEntryPoint(
    ShaderStage stage, const QString& name) const
{
    if (!name.isEmpty()) {
        for (const CompiledEntryPoint& entry : m_entryPoints)
            if (entry.stage == stage && entry.name == name)
                return &entry;
        return nullptr;
    }
    const QString preferred = stage == ShaderStage::Vertex ? QStringLiteral("vsMain")
        : stage == ShaderStage::Fragment                   ? QStringLiteral("psMain")
                                                           : QString();
    for (const CompiledEntryPoint& entry : m_entryPoints)
        if (entry.stage == stage && entry.name == preferred)
            return &entry;
    for (const CompiledEntryPoint& entry : m_entryPoints)
        if (entry.stage == stage)
            return &entry;
    return nullptr;
}

const QShader& ShaderDocument::vertexShader() const
{
    static const QShader invalid;
    const auto* entry = findEntryPoint(ShaderStage::Vertex);
    return entry ? entry->shader : invalid;
}

const QShader& ShaderDocument::fragmentShader() const
{
    static const QShader invalid;
    const auto* entry = findEntryPoint(ShaderStage::Fragment);
    return entry ? entry->shader : invalid;
}

ShaderDocument::ShaderDocument(QObject* parent)
    : QObject(parent)
    , m_compiler(this)
    , m_parameters(this)
    , m_graph(this)
{
    m_compileTimer.setSingleShot(true);
    m_compileTimer.setInterval(180);
    connect(&m_compileTimer, &QTimer::timeout, this, &ShaderDocument::compileNow);
    connect(&m_dependencyWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString&) {
        // Editors write files by replacement on many platforms, so compileNow re-adds
        // every resolved dependency after Slang has loaded the new contents.
        m_compileTimer.start();
    });
    connect(&m_parameters, &ShaderParameterModel::packedRangeChanged, this,
        [this](int, int) { m_graph.setPayload(m_valuesNode, m_parameters.packedBytes()); });
    initializeGraph();
}

void ShaderDocument::initializeGraph()
{
    m_sourceNode = m_graph.ensureNode(QStringLiteral("source:user"), NodeKind::Source);
    m_entryNode = m_graph.ensureNode(QStringLiteral("shader:entrypoints"), NodeKind::EntryPoint);
    m_uiNode = m_graph.ensureNode(QStringLiteral("ui:schema"), NodeKind::UiSchema);
    m_layoutNode
        = m_graph.ensureNode(QStringLiteral("shader:parameter-layout"), NodeKind::ParameterLayout);
    m_valuesNode
        = m_graph.ensureNode(QStringLiteral("shader:parameter-values"), NodeKind::ParameterValues);
    m_pipelineNode = m_graph.ensureNode(QStringLiteral("rhi:pipeline"), NodeKind::Pipeline);
    m_graph.setDependencies(m_entryNode, { m_sourceNode });
    // ParameterLayout is a semantic compiler output, not a child hash of source.
    // This is what lets implementation-only shader edits avoid rebuilding the UI.
    m_graph.setDependencies(m_uiNode, { m_layoutNode });
    m_graph.setDependencies(m_valuesNode, { m_layoutNode });
    m_graph.setDependencies(m_pipelineNode, { m_entryNode, m_layoutNode });
    m_graph.markAllClean();
}

void ShaderDocument::setFileUrl(const QUrl& url)
{
    if (m_fileUrl == url)
        return;
    m_fileUrl = url;
    emit fileUrlChanged();
}

void ShaderDocument::setSource(const QString& source)
{
    if (m_source == source)
        return;
    m_source = source;
    if (!m_dirty) {
        m_dirty = true;
        emit dirtyChanged();
    }
    m_graph.setPayload(m_sourceNode, source.toUtf8());
    emit sourceChanged();
    if (m_live)
        m_compileTimer.start();
}

void ShaderDocument::setLive(bool live)
{
    if (m_live == live)
        return;
    m_live = live;
    emit liveChanged();
    if (live && !m_source.isEmpty())
        m_compileTimer.start();
}

bool ShaderDocument::load()
{
    if (!m_fileUrl.isLocalFile())
        return false;
    QFile f(m_fileUrl.toLocalFile());
    if (!f.open(QIODevice::ReadOnly)) {
        setDiagnostics(f.errorString());
        return false;
    }
    setSource(QString::fromUtf8(f.readAll()));
    markSourceClean();
    return true;
}

bool ShaderDocument::save()
{
    if (!m_fileUrl.isLocalFile())
        return false;
    QSaveFile f(m_fileUrl.toLocalFile());
    if (!f.open(QIODevice::WriteOnly)) {
        setDiagnostics(f.errorString());
        return false;
    }
    f.write(m_source.toUtf8());
    if (!f.commit()) {
        setDiagnostics(f.errorString());
        return false;
    }
    if (m_dirty) {
        m_dirty = false;
        emit dirtyChanged();
    }
    return true;
}

void ShaderDocument::compile()
{
    m_compileTimer.stop();
    compileNow();
}

void ShaderDocument::markSourceClean()
{
    if (!m_dirty)
        return;
    m_dirty = false;
    emit dirtyChanged();
}

void ShaderDocument::compileNow()
{
    if (m_compiling)
        return;
    m_compiling = true;
    emit compilingChanged();
    const QString virtualPath
        = m_fileUrl.isLocalFile() ? m_fileUrl.toLocalFile() : QStringLiteral("user_shader.slang");
    if (m_fileUrl.isLocalFile())
        m_compiler.setSearchPaths({ QFileInfo(virtualPath).absolutePath() });
    QElapsedTimer timer;
    timer.start();
    const CompileResult r = m_compiler.compileProgram(m_source, virtualPath);
    m_compileSucceeded = r.ok;
    m_lastCompileMs = int(timer.elapsed());
    m_compiling = false;
    emit compilingChanged();
    setDiagnostics(r.diagnostics);
    m_importedDependencies = r.dependencies;
    m_resources = r.resources;
    if (!m_dependencyWatcher.files().isEmpty())
        m_dependencyWatcher.removePaths(m_dependencyWatcher.files());
    QStringList watchedFiles;
    for (const SourceDependency& dependency : r.dependencies) {
        const QFileInfo info(dependency.path);
        if (info.isFile())
            watchedFiles.push_back(info.absoluteFilePath());
    }
    if (!watchedFiles.isEmpty())
        m_dependencyWatcher.addPaths(watchedFiles);
    m_moduleNodes.clear();
    for (const SourceDependency& dependency : r.dependencies) {
        const NodeId node
            = m_graph.ensureNode(QStringLiteral("module:") + dependency.identity, NodeKind::Module);
        m_graph.setPayload(node, dependency.source);
        m_moduleNodes.push_back(node);
    }
    for (const SourceDependency& dependency : r.dependencies) {
        QList<NodeId> imports;
        for (const QString& identity : dependency.imports) {
            const NodeId imported = m_graph.nodeId(QStringLiteral("module:") + identity);
            if (imported)
                imports.push_back(imported);
        }
        m_graph.setDependencies(
            m_graph.nodeId(QStringLiteral("module:") + dependency.identity), imports);
    }
    QList<NodeId> entryDependencies { m_sourceNode };
    entryDependencies.append(m_moduleNodes);
    m_graph.setDependencies(m_entryNode, entryDependencies);
    emit dependenciesChanged();
    if (!r.ok) {
        emit compileFailed(r.diagnostics);
        return;
    }

    const QByteArray oldLayout = m_graph.payload(m_layoutNode);
    const QByteArray oldUiSchema = m_graph.payload(m_uiNode);
    if (oldLayout != r.parameterLayoutDigest || oldUiSchema != r.uiSchemaDigest) {
        m_parameters.setDescriptors(r.parameters, r.parameterByteSize);
        m_graph.setPayload(m_layoutNode, r.parameterLayoutDigest);
        m_graph.setPayload(m_uiNode, r.uiSchemaDigest);
        m_graph.setPayload(m_valuesNode, m_parameters.packedBytes());
    }

    m_entryPoints = r.entryPoints;
    m_parameterBinding = r.parameterBinding;

    // Collect generated backend code for every reflected entry point.
    m_generated.clear();
    m_generatedTargets.clear();
    for (const QString& name : { QStringLiteral("HLSL"), QStringLiteral("GLSL"),
             QStringLiteral("SPIR-V"), QStringLiteral("Metal") }) {
        QString combined;
        for (const CompiledEntryPoint& entry : r.entryPoints) {
            const QString code = entry.generated.value(name);
            if (code.isEmpty())
                continue;
            combined += QStringLiteral("// ===== %1 entry: %2 =====\n\n")
                            .arg(shaderStageName(entry.stage), entry.name)
                + code + QStringLiteral("\n\n");
        }
        if (combined.isEmpty())
            continue;
        m_generated.insert(name, combined);
        m_generatedTargets.push_back(name);
    }

    QByteArray entryIdentity;
    for (const CompiledEntryPoint& entry : r.entryPoints)
        entryIdentity += entry.identity;
    m_graph.setPayload(m_entryNode, entryIdentity);
    m_graph.setPayload(m_pipelineNode, QByteArray("graphics"));
    emit shaderPackageChanged();
    emit compiled();
}

void ShaderDocument::setDiagnostics(QString text)
{
    if (m_diagnostics == text)
        return;
    m_diagnostics = std::move(text);
    emit diagnosticsChanged();
}

} // namespace miskeyed::workbench::slang_rhi
