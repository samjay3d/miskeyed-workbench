#include <miskeyed/workbench/slang_rhi/ShaderDocument.h>
#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>

namespace miskeyed::workbench::slang_rhi {

ShaderDocument::ShaderDocument(QObject* parent)
    : QObject(parent)
    , m_compiler(this)
    , m_parameters(this)
    , m_graph(this)
{
    m_compileTimer.setSingleShot(true);
    m_compileTimer.setInterval(180);
    connect(&m_compileTimer, &QTimer::timeout, this, &ShaderDocument::compileNow);
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
    return true;
}

void ShaderDocument::compile()
{
    m_compileTimer.stop();
    compileNow();
}

void ShaderDocument::compileNow()
{
    if (m_compiling)
        return;
    m_compiling = true;
    emit compilingChanged();
    const QString virtualPath
        = m_fileUrl.isLocalFile() ? m_fileUrl.toLocalFile() : QStringLiteral("user_shader.slang");
    QElapsedTimer timer;
    timer.start();
    const CompileResult r = m_compiler.compileFullscreen(m_source, virtualPath);
    m_lastCompileMs = int(timer.elapsed());
    m_compiling = false;
    emit compilingChanged();
    setDiagnostics(r.diagnostics);
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

    m_vertexShader = r.vertex.shader;
    m_fragmentShader = r.fragment.shader;
    m_parameterBinding = r.parameterBinding;

    // Collect generated backend code (vertex + fragment) for the editor's output viewer.
    m_generated.clear();
    m_generatedTargets.clear();
    for (const QString& name : { QStringLiteral("HLSL"), QStringLiteral("GLSL"),
             QStringLiteral("SPIR-V"), QStringLiteral("Metal") }) {
        const QString vs = r.vertex.generated.value(name);
        const QString fs = r.fragment.generated.value(name);
        if (vs.isEmpty() && fs.isEmpty())
            continue;
        QString combined;
        combined += QStringLiteral("// ===== Vertex entry: vsMain =====\n\n") + vs;
        combined += QStringLiteral("\n\n// ===== Fragment entry: psMain =====\n\n") + fs;
        m_generated.insert(name, combined);
        m_generatedTargets.push_back(name);
    }

    m_graph.setPayload(m_entryNode, r.vertex.entryPointHash + r.fragment.entryPointHash);
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
