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
    emit bindingChanged(document);
    return true;
}

void ShaderToySession::removeDocument(ShaderDocument* document)
{
    if (m_shader == document)
        bindShader(nullptr);
}

} // namespace miskeyed::workbench::slang_rhi
