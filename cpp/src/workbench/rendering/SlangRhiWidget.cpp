#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/rendering/RenderPass.h>
#include <miskeyed/workbench/core/ViewportCamera.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <rhi/qrhi.h>
#include <QColor>
#include <QMouseEvent>
#include <QWheelEvent>
#include <limits>
#include <cmath>
#include <vector>
#include <algorithm>

namespace miskeyed::workbench::slang_rhi {

class SlangRhiWidgetPrivate {
public:
    // Post-process / main pass drawn to the widget's backbuffer.
    RenderPass main;
    // Optional scene pre-pass drawn to the offscreen G-buffer texture.
    RenderPass scene;
    // Offscreen color target ("G-buffer") the scene pass writes and the main pass samples.
    std::unique_ptr<QRhiTexture> sceneTex;
    std::unique_ptr<QRhiTextureRenderTarget> sceneRt;
    std::unique_ptr<QRhiRenderPassDescriptor> sceneRp;
    std::unique_ptr<QRhiSampler> sampler;
    QSize offscreenSize;

    // Deferred release: QRhi may still reference a buffer/SRB/pipeline/texture from a
    // command buffer recorded up to "max frame latency" (2) frames ago. Destroying such a
    // resource synchronously (e.g. when a shader's uniform size changes and its buffer is
    // recreated) makes the D3D11 backend bind freed resources -> access violation. Instead
    // we park replaced resources here and free them a few frames later, once no in-flight
    // command buffer can reference them.
    struct PendingRelease {
        std::unique_ptr<QRhiResource> res;
        int framesLeft;
    };
    std::vector<PendingRelease> graveyard;

    void retire(std::unique_ptr<QRhiResource> res)
    {
        if (res)
            graveyard.push_back({ std::move(res), 4 });
    }
    void tickGraveyard()
    {
        for (auto& p : graveyard)
            --p.framesLeft;
        graveyard.erase(std::remove_if(graveyard.begin(), graveyard.end(),
                            [](const PendingRelease& p) { return p.framesLeft <= 0; }),
            graveyard.end());
    }

    void clear()
    {
        main.reset();
        scene.reset();
        sceneTex.reset();
        sceneRt.reset();
        sceneRp.reset();
        sampler.reset();
        graveyard.clear();
        offscreenSize = {};
    }
};

SlangRhiWidget::SlangRhiWidget(QWidget* parent)
    : QRhiWidget(parent)
    , m_backend(rendering::RhiBackendPolicy::defaultBackend())
    , d(std::make_unique<SlangRhiWidgetPrivate>())
{
    setApi(rendering::RhiBackendPolicy::api(m_backend));
    setColorBufferFormat(QRhiWidget::TextureFormat::RGBA16F);
}
SlangRhiWidget::~SlangRhiWidget() = default;

QString SlangRhiWidget::backendName() const
{
    const auto resolved = m_backend == rendering::RhiBackend::Auto
        ? rendering::RhiBackendPolicy::platformDefault()
        : m_backend;
    return rendering::RhiBackendPolicy::name(resolved);
}

void SlangRhiWidget::setDocument(ShaderDocument* document)
{
    if (m_document == document)
        return;
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document) {
        connect(m_document, &ShaderDocument::shaderPackageChanged, this,
            &SlangRhiWidget::onShaderChanged);
        connect(m_document->parameters(), &ShaderParameterModel::packedRangeChanged, this,
            &SlangRhiWidget::onParameterRangeChanged);
        d->main.pendingUniforms = m_document->parameters()->packedBytes();
    }
    d->main.pipelineDirty = true;
    d->main.uniformsDirty = true;
    emit documentChanged();
    applyTimeContext();
    update();
}

void SlangRhiWidget::setScenePass(ShaderDocument* sceneDocument)
{
    if (m_scenePass == sceneDocument)
        return;
    if (m_scenePass)
        disconnect(m_scenePass, nullptr, this, nullptr);
    m_scenePass = sceneDocument;
    if (m_scenePass) {
        connect(m_scenePass, &ShaderDocument::shaderPackageChanged, this,
            &SlangRhiWidget::onScenePassChanged);
        connect(m_scenePass->parameters(), &ShaderParameterModel::packedRangeChanged, this,
            &SlangRhiWidget::onScenePassRangeChanged);
        d->scene.pendingUniforms = m_scenePass->parameters()->packedBytes();
    }
    // The main pass gains/loses a texture binding, so its pipeline must be rebuilt. Retire
    // the old resources instead of freeing them now in case a frame is still in flight.
    d->scene.pipelineDirty = true;
    d->scene.uniformsDirty = true;
    d->retire(std::move(d->main.srb));
    d->retire(std::move(d->main.pipeline));
    d->main.pipelineDirty = true;
    applyTimeContext();
    update();
}

void SlangRhiWidget::setTimeContext(core::TimeContext* context)
{
    if (m_timeContext == context)
        return;
    if (m_timeContext)
        disconnect(m_timeContext, nullptr, this, nullptr);
    m_timeContext = context;
    if (m_timeContext)
        connect(
            m_timeContext, &core::TimeContext::changed, this, &SlangRhiWidget::applyTimeContext);
    applyTimeContext();
}

void SlangRhiWidget::applyTimeContext()
{
    if (!m_timeContext)
        return;
    auto apply = [this](ShaderDocument* document) {
        if (!document)
            return;
        auto* parameters = document->parameters();
        parameters->setValue(
            QString::fromLatin1(core::TimeBinding::time), float(m_timeContext->timeSeconds()));
        parameters->setValue(QString::fromLatin1(core::TimeBinding::deltaTime),
            float(m_timeContext->deltaSeconds()));
        parameters->setValue(QString::fromLatin1(core::TimeBinding::frame),
            uint(
                qMin<quint64>(m_timeContext->evaluationIndex(), std::numeric_limits<uint>::max())));
        parameters->setValue(QString::fromLatin1(core::TimeBinding::frameRate),
            float(m_timeContext->current().rate));
    };
    apply(m_scenePass);
    apply(m_document);
    update();
}

void SlangRhiWidget::setExposure(float value)
{
    if (qFuzzyCompare(m_exposure, value))
        return;
    m_exposure = value;
    emit exposureChanged();
    update();
}

void SlangRhiWidget::onShaderChanged()
{
    applyTimeContext();
    d->main.pipelineDirty = true;
    d->main.pendingUniforms = m_document ? m_document->parameters()->packedBytes() : QByteArray();
    d->main.uniformsDirty = true;
    update();
}

void SlangRhiWidget::onParameterRangeChanged(int, int)
{
    if (!m_document)
        return;
    d->main.pendingUniforms = m_document->parameters()->packedBytes();
    d->main.uniformsDirty = true;
    update();
}

void SlangRhiWidget::onScenePassChanged()
{
    applyTimeContext();
    d->scene.pipelineDirty = true;
    d->scene.pendingUniforms
        = m_scenePass ? m_scenePass->parameters()->packedBytes() : QByteArray();
    d->scene.uniformsDirty = true;
    update();
}

void SlangRhiWidget::onScenePassRangeChanged(int, int)
{
    if (!m_scenePass)
        return;
    d->scene.pendingUniforms = m_scenePass->parameters()->packedBytes();
    d->scene.uniformsDirty = true;
    update();
}

namespace {

    // Build (if dirty) the uniform buffer, resource bindings and pipeline for one pass.
    // `sampleTex` (when non-null) adds a combined image sampler at SRB slot 1 so a
    // post-process shader can read the scene texture (HLSL t1/s1 via QRhi's fallback).
    // Replaced resources are retired into `priv`'s graveyard rather than freed immediately,
    // so an in-flight command buffer never binds a destroyed buffer/SRB/pipeline.
    bool buildPass(QRhi* r, SlangRhiWidgetPrivate& priv, RenderPass& pass, ShaderDocument* doc,
        QRhiRenderPassDescriptor* rp, QRhiTexture* sampleTex, QRhiSampler* sampler)
    {
        const int uniformSize = qMax(16, int(pass.pendingUniforms.size()));
        if (!pass.uniforms || pass.uniforms->size() != uniformSize) {
            priv.retire(std::move(pass.uniforms));
            pass.uniforms.reset(
                r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformSize));
            if (!pass.uniforms->create())
                return false;
            priv.retire(std::move(pass.srb));
            priv.retire(std::move(pass.pipeline));
            pass.pipelineDirty = true;
            pass.uniformsDirty = true;
        }

        const int wantedBinding = doc ? doc->parameterBinding() : 0;
        if (pass.boundSlot != wantedBinding) {
            priv.retire(std::move(pass.srb));
            priv.retire(std::move(pass.pipeline));
            pass.pipelineDirty = true;
            pass.boundSlot = wantedBinding;
        }

        if (!pass.srb) {
            pass.srb.reset(r->newShaderResourceBindings());
            QList<QRhiShaderResourceBinding> bindings;
            bindings << QRhiShaderResourceBinding::uniformBuffer(wantedBinding,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                pass.uniforms.get());
            if (sampleTex && sampler) {
                bindings << QRhiShaderResourceBinding::sampledTexture(
                    1, QRhiShaderResourceBinding::FragmentStage, sampleTex, sampler);
            }
            pass.srb->setBindings(bindings.cbegin(), bindings.cend());
            if (!pass.srb->create())
                return false;
        }

        if (pass.pipelineDirty && doc && doc->vertexShader().isValid()
            && doc->fragmentShader().isValid()) {
            auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(r->newGraphicsPipeline());
            pipeline->setShaderStages({
                { QRhiShaderStage::Vertex, doc->vertexShader() },
                { QRhiShaderStage::Fragment, doc->fragmentShader() },
            });
            pipeline->setVertexInputLayout(QRhiVertexInputLayout());
            pipeline->setShaderResourceBindings(pass.srb.get());
            pipeline->setRenderPassDescriptor(rp);
            if (!pipeline->create())
                return false;
            priv.retire(std::move(pass.pipeline));
            pass.pipeline = std::move(pipeline);
            pass.pipelineDirty = false;
        }
        return true;
    }

} // namespace

void SlangRhiWidget::initialize(QRhiCommandBuffer*)
{
    if (!m_reportedBackend && rhi()) {
        m_reportedBackend = true;
        emit backendInitialized(backendName(), QString::fromUtf8(rhi()->driverInfo().deviceName));
    }
    auto* r = rhi();
    if (!r || !renderTarget())
        return;

    const bool twoPass = m_scenePass && m_scenePass->vertexShader().isValid()
        && m_scenePass->fragmentShader().isValid();

    // (Re)create the offscreen G-buffer target when it is needed and out of date.
    if (twoPass) {
        const QSize size = renderTarget()->pixelSize();
        if (!d->sceneTex || d->offscreenSize != size) {
            // Retire the old target: the main pass SRB still samples this texture and a
            // frame may still be in flight, so it must outlive the current command buffer.
            d->retire(std::move(d->sceneTex));
            d->retire(std::move(d->sceneRt));
            d->retire(std::move(d->sceneRp));
            d->sceneTex.reset(r->newTexture(QRhiTexture::RGBA16F, size, 1,
                QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
            if (!d->sceneTex->create()) {
                emit gpuError(QStringLiteral("Failed to create offscreen scene texture."));
                return;
            }
            QRhiTextureRenderTargetDescription rtDesc({ QRhiColorAttachment(d->sceneTex.get()) });
            d->sceneRt.reset(r->newTextureRenderTarget(rtDesc));
            d->sceneRp.reset(d->sceneRt->newCompatibleRenderPassDescriptor());
            d->sceneRt->setRenderPassDescriptor(d->sceneRp.get());
            if (!d->sceneRt->create()) {
                emit gpuError(QStringLiteral("Failed to create offscreen render target."));
                return;
            }
            d->offscreenSize = size;
            d->scene.pipelineDirty = true;
            // The main pass samples the recreated texture; force it to rebind and rebuild.
            d->retire(std::move(d->main.srb));
            d->retire(std::move(d->main.pipeline));
            d->main.pipelineDirty = true;
        }
        if (!d->sampler) {
            d->sampler.reset(r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
            if (!d->sampler->create()) {
                emit gpuError(QStringLiteral("Failed to create sampler."));
                return;
            }
        }
        if (!buildPass(r, *d, d->scene, m_scenePass, d->sceneRp.get(), nullptr, nullptr)) {
            emit gpuError(QStringLiteral("Failed to build scene pass."));
            return;
        }
        if (!buildPass(r, *d, d->main, m_document, renderTarget()->renderPassDescriptor(),
                d->sceneTex.get(), d->sampler.get())) {
            emit gpuError(QStringLiteral("Failed to build post-process pass."));
            return;
        }
    } else {
        d->retire(std::move(d->sceneTex));
        d->retire(std::move(d->sceneRt));
        d->retire(std::move(d->sceneRp));
        d->offscreenSize = {};
        if (!buildPass(r, *d, d->main, m_document, renderTarget()->renderPassDescriptor(), nullptr,
                nullptr)) {
            emit gpuError(QStringLiteral("Failed to build pass."));
            return;
        }
    }
}

void SlangRhiWidget::render(QRhiCommandBuffer* cb)
{
    if (!cb || !renderTarget())
        return;
    // A shader recompile sets pipelineDirty and calls update(), which schedules
    // render() but not initialize(); rebuild GPU resources here too so changes show
    // immediately instead of only after a resize.
    initialize(cb);

    auto* r = rhi();
    const bool twoPass = m_scenePass && d->sceneRt && d->scene.pipeline;

    auto packUniforms = [&](RenderPass& pass, QRhiResourceUpdateBatch* batch) {
        if (!pass.uniformsDirty || !pass.uniforms)
            return;
        QByteArray bytes = pass.pendingUniforms;
        if (bytes.size() < int(pass.uniforms->size()))
            bytes.append(QByteArray(int(pass.uniforms->size()) - bytes.size(), '\0'));
        batch->updateDynamicBuffer(pass.uniforms.get(), 0, bytes.size(), bytes.constData());
        pass.uniformsDirty = false;
    };

    QRhiResourceUpdateBatch* updates = r->nextResourceUpdateBatch();
    if (twoPass)
        packUniforms(d->scene, updates);
    packUniforms(d->main, updates);

    if (twoPass) {
        // Pass 1 — render the scene into the offscreen G-buffer.
        cb->beginPass(
            d->sceneRt.get(), QColor::fromRgbF(0.025, 0.027, 0.032, 1.0), { 1.0f, 0 }, updates);
        cb->setGraphicsPipeline(d->scene.pipeline.get());
        cb->setViewport(
            { 0, 0, float(d->offscreenSize.width()), float(d->offscreenSize.height()) });
        cb->setShaderResources(d->scene.srb.get());
        cb->draw(3);
        cb->endPass();
        updates = nullptr;
    }

    // Final pass — draw to the widget's backbuffer (post-process samples the G-buffer).
    cb->beginPass(renderTarget(), QColor::fromRgbF(0.025, 0.027, 0.032, 1.0), { 1.0f, 0 }, updates);
    if (d->main.pipeline) {
        cb->setGraphicsPipeline(d->main.pipeline.get());
        cb->setViewport({ 0, 0, float(renderTarget()->pixelSize().width()),
            float(renderTarget()->pixelSize().height()) });
        cb->setShaderResources(d->main.srb.get());
        cb->draw(3);
    }
    cb->endPass();

    if (!m_reportedFrame && d->main.pipeline && (!m_scenePass || twoPass)) {
        m_reportedFrame = true;
        emit frameRendered(backendName(), QString::fromUtf8(r->driverInfo().deviceName));
    }

    // Age out resources retired this frame; they are freed once no in-flight command
    // buffer (max frame latency 2) can still reference them.
    d->tickGraveyard();
}

void SlangRhiWidget::releaseResources()
{
    d->clear();
}

// -----------------------------------------------------------------------------
// Houdini-style camera navigation
// -----------------------------------------------------------------------------
// The camera lives in the shader as `cam*` uniforms. We only add/subtract deltas
// here, so a shader without a given uniform simply ignores that gesture.

bool SlangRhiWidget::nudgeParam(const char* name, float delta)
{
    if (delta == 0.0f)
        return false;
    const QString key = QString::fromLatin1(name);
    auto apply = [&](ShaderDocument* doc) -> bool {
        if (!doc)
            return false;
        const QVariant current = doc->parameters()->value(key);
        if (!current.isValid())
            return false;
        return doc->parameters()->setValue(key, current.toFloat() + delta);
    };
    // Nudge both this widget's document and its scene pre-pass: the camera uniforms may
    // live in either (a post-process shader samples the scene but has no camera itself).
    bool changed = apply(m_document);
    changed = apply(m_scenePass) || changed;
    return changed;
}

void SlangRhiWidget::mousePressEvent(QMouseEvent* event)
{
    emit activated();
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton
        || event->button() == Qt::RightButton) {
        m_dragging = true;
        m_lastDragPos = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QRhiWidget::mousePressEvent(event);
}

void SlangRhiWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging) {
        QRhiWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF pos = event->position();
    const QPointF delta = pos - m_lastDragPos;
    m_lastDragPos = pos;

    const Qt::MouseButtons buttons = event->buttons();
    if (buttons & Qt::LeftButton) {
        // Tumble: horizontal drag orbits (yaw), vertical drag pitches.
        nudgeParam(core::ViewportCameraBinding::yaw, float(-delta.x()) * 0.010f);
        nudgeParam(core::ViewportCameraBinding::pitch, float(-delta.y()) * 0.010f);
    } else if (buttons & Qt::MiddleButton) {
        // Pan: track the look-at point in screen space.
        nudgeParam(core::ViewportCameraBinding::panX, float(-delta.x()) * 0.006f);
        nudgeParam(core::ViewportCameraBinding::panY, float(delta.y()) * 0.006f);
    } else if (buttons & Qt::RightButton) {
        // Dolly: drag up moves closer, drag down pulls back.
        nudgeParam(core::ViewportCameraBinding::distance, float(delta.y()) * 0.02f);
    }
    event->accept();
}

void SlangRhiWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragging) {
        m_dragging = false;
        unsetCursor();
        event->accept();
        return;
    }
    QRhiWidget::mouseReleaseEvent(event);
}

void SlangRhiWidget::wheelEvent(QWheelEvent* event)
{
    const float steps = float(event->angleDelta().y()) / 120.0f;
    if (steps != 0.0f && nudgeParam(core::ViewportCameraBinding::distance, -steps * 0.3f)) {
        event->accept();
        return;
    }
    QRhiWidget::wheelEvent(event);
}

} // namespace miskeyed::workbench::slang_rhi
