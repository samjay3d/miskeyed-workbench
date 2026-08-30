#pragma once

#include <miskeyed/workbench/Export.h>
#include <QList>
#include <QObject>
#include <QString>

class QWidget;

namespace miskeyed::workbench::slang_rhi {
class RenderToySession;
class ShaderToySession;
class ShaderWorkspace;
class SlangRhiWidget;

// A contribution describes reusable views of a session. It is deliberately not the
// session identity and is not restricted to one QWidget; layout presets decide which
// primary views are visible. Workspace documents, focus, and time remain shared.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchToolContribution : public QObject {
public:
    using QObject::QObject;
    ~WorkbenchToolContribution() override = default;
    virtual QString toolId() const = 0;
    virtual QString title() const = 0;
    virtual QObject* session() const = 0;
    virtual QList<QWidget*> primaryViews() const = 0;
    virtual QString statusSummary() const = 0;
};

// Each mode has its own small factory signature. Adding Lookdev or ANARI adds another
// factory; it does not grow a central function with every future session and viewport.
[[nodiscard]] MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchToolContribution*
createRenderToyContribution(QWidget* parent, ShaderWorkspace* workspace, RenderToySession* session,
    SlangRhiWidget* sceneViewport, SlangRhiWidget* postViewport);
[[nodiscard]] MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchToolContribution*
createShaderToyContribution(QWidget* parent, ShaderWorkspace* workspace, ShaderToySession* session,
    SlangRhiWidget* viewport);
} // namespace miskeyed::workbench::slang_rhi
