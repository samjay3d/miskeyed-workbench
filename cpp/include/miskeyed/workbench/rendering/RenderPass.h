#pragma once

#include <QByteArray>
#include <memory>

class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;

namespace miskeyed::workbench::slang_rhi {

// The reusable state for one educational Render Toy draw pass. Pass composition and
// target ownership remain in the viewport; this type owns only pass-local resources.
struct RenderPass final {
    std::unique_ptr<QRhiBuffer> uniforms;
    std::unique_ptr<QRhiShaderResourceBindings> srb;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline;
    QByteArray pendingUniforms;
    bool pipelineDirty = true;
    bool uniformsDirty = true;
    int boundSlot = -1;

    RenderPass();
    ~RenderPass();
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    void reset();
};

} // namespace miskeyed::workbench::slang_rhi
