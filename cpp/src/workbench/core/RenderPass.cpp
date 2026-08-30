#include <miskeyed/workbench/slang_rhi/RenderPass.h>

#include <rhi/qrhi.h>

namespace miskeyed::workbench::slang_rhi {

RenderPass::RenderPass() = default;
RenderPass::~RenderPass() = default;
RenderPass::RenderPass(RenderPass&&) noexcept = default;
RenderPass& RenderPass::operator=(RenderPass&&) noexcept = default;

void RenderPass::reset()
{
    pipeline.reset();
    srb.reset();
    uniforms.reset();
    pipelineDirty = true;
    uniformsDirty = true;
    boundSlot = -1;
}

} // namespace miskeyed::workbench::slang_rhi
