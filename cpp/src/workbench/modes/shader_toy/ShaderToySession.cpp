#include <miskeyed/workbench/modes/shader_toy/ShaderToySession.h>

namespace miskeyed::workbench::slang_rhi {

ShaderToySession::ShaderToySession(QObject* parent)
    : QObject(parent)
{
}

void ShaderToySession::bindShader(ShaderDocument* document)
{
    if (m_shader == document)
        return;
    m_shader = document;
    emit bindingChanged(document);
}

void ShaderToySession::removeDocument(ShaderDocument* document)
{
    if (m_shader == document)
        bindShader(nullptr);
}

} // namespace miskeyed::workbench::slang_rhi
