#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>

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
    const auto* vertex = document ? document->findEntryPoint(ShaderStage::Vertex) : nullptr;
    const auto* fragment = document ? document->findEntryPoint(ShaderStage::Fragment) : nullptr;
    m_sceneVertex = vertex ? vertex->name : QString();
    m_sceneFragment = fragment ? fragment->name : QString();
    emit bindingsChanged(m_scene, m_post);
}

void RenderToySession::bindPost(ShaderDocument* document)
{
    if (m_post == document)
        return;
    m_post = document;
    const auto* vertex = document ? document->findEntryPoint(ShaderStage::Vertex) : nullptr;
    const auto* fragment = document ? document->findEntryPoint(ShaderStage::Fragment) : nullptr;
    m_postVertex = vertex ? vertex->name : QString();
    m_postFragment = fragment ? fragment->name : QString();
    emit bindingsChanged(m_scene, m_post);
}

bool RenderToySession::selectSceneEntryPoints(const QString& vertex, const QString& fragment)
{
    if (!m_scene || !m_scene->findEntryPoint(ShaderStage::Vertex, vertex)
        || !m_scene->findEntryPoint(ShaderStage::Fragment, fragment))
        return false;
    m_sceneVertex = vertex;
    m_sceneFragment = fragment;
    emit entryPointsChanged();
    return true;
}

bool RenderToySession::selectPostEntryPoints(const QString& vertex, const QString& fragment)
{
    if (!m_post || !m_post->findEntryPoint(ShaderStage::Vertex, vertex)
        || !m_post->findEntryPoint(ShaderStage::Fragment, fragment))
        return false;
    m_postVertex = vertex;
    m_postFragment = fragment;
    emit entryPointsChanged();
    return true;
}

void RenderToySession::resolveEntryPoints()
{
    auto resolve = [](ShaderDocument* document, QString& vertexName, QString& fragmentName) {
        const auto* vertex = document ? document->findEntryPoint(ShaderStage::Vertex) : nullptr;
        const auto* fragment = document ? document->findEntryPoint(ShaderStage::Fragment) : nullptr;
        vertexName = vertex ? vertex->name : QString();
        fragmentName = fragment ? fragment->name : QString();
    };
    resolve(m_scene, m_sceneVertex, m_sceneFragment);
    resolve(m_post, m_postVertex, m_postFragment);
    emit entryPointsChanged();
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
