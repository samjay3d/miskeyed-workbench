#include <miskeyed/workbench/modes/shader_toy/ShaderToySession.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>

namespace miskeyed::workbench::slang_rhi {

ShaderToySession::ShaderToySession(QObject* parent)
    : QObject(parent)
{
}

bool ShaderToySession::canBindShader(ShaderDocument* document) const
{
    if (!document)
        return true;
    // An uncompiled document remains bindable so live compilation can establish the
    // artifact. Once compiled, Shader Toy requires the fullscreen graphics contract.
    return document->lastCompileMs() < 0
        || (document->vertexShader().isValid() && document->fragmentShader().isValid());
}

bool ShaderToySession::bindShader(ShaderDocument* document)
{
    if (m_shader == document)
        return true;
    if (!canBindShader(document)) {
        emit bindingRejected(document,
            QStringLiteral("ShaderToy requires compiled vertex and fragment entry points."));
        return false;
    }
    m_shader = document;
    resolveEntryPoints();
    emit bindingChanged(document);
    return true;
}

bool ShaderToySession::selectEntryPoints(const QString& vertex, const QString& fragment)
{
    if (!m_shader || !m_shader->findEntryPoint(ShaderStage::Vertex, vertex)
        || !m_shader->findEntryPoint(ShaderStage::Fragment, fragment))
        return false;
    if (m_vertexEntry == vertex && m_fragmentEntry == fragment)
        return true;
    m_vertexEntry = vertex;
    m_fragmentEntry = fragment;
    emit entryPointsChanged(vertex, fragment);
    return true;
}

void ShaderToySession::resolveEntryPoints()
{
    const auto* vertex = m_shader ? m_shader->findEntryPoint(ShaderStage::Vertex) : nullptr;
    const auto* fragment = m_shader ? m_shader->findEntryPoint(ShaderStage::Fragment) : nullptr;
    const QString vertexName = vertex ? vertex->name : QString();
    const QString fragmentName = fragment ? fragment->name : QString();
    if (m_vertexEntry == vertexName && m_fragmentEntry == fragmentName)
        return;
    m_vertexEntry = vertexName;
    m_fragmentEntry = fragmentName;
    emit entryPointsChanged(m_vertexEntry, m_fragmentEntry);
}

void ShaderToySession::removeDocument(ShaderDocument* document)
{
    if (m_shader == document)
        bindShader(nullptr);
}

} // namespace miskeyed::workbench::slang_rhi
