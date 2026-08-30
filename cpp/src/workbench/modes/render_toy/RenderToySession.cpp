#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>

namespace miskeyed::workbench::slang_rhi {

RenderToySession::RenderToySession(QObject* parent)
    : QObject(parent)
    , m_timeContext(new core::TimeContext(this))
    , m_timeTransport(new core::TimeTransport(this))
{
    m_timeTransport->setContext(m_timeContext);
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
