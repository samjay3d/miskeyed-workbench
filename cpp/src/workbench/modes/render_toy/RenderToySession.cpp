#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>

namespace miskeyed::workbench::slang_rhi {

RenderToySession::RenderToySession(QObject* parent)
    : QObject(parent)
{
}

void RenderToySession::setEvaluationContext(
    core::TimeContext* context, core::TimeTransport* transport)
{
    m_timeContext = context;
    m_timeTransport = transport;
}

void RenderToySession::bindScene(ShaderDocument* document)
{
    if (m_scene == document)
        return;
    m_scene = document;
    emit bindingsChanged(m_scene, m_post);
}

void RenderToySession::bindPost(ShaderDocument* document)
{
    if (m_post == document)
        return;
    m_post = document;
    emit bindingsChanged(m_scene, m_post);
}

void RenderToySession::removeDocument(ShaderDocument* document)
{
    bool changed = false;
    if (m_scene == document) {
        m_scene = nullptr;
        changed = true;
    }
    if (m_post == document) {
        m_post = nullptr;
        changed = true;
    }
    if (changed)
        emit bindingsChanged(m_scene, m_post);
}

} // namespace miskeyed::workbench::slang_rhi
