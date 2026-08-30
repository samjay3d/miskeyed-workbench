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

// The shell consumes only this interface. A concrete tool session owns its binding UI
// and data binding; the Workspace remains the owner of documents, focus, and time.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchToolUiSession : public QObject {
public:
    using QObject::QObject;
    ~WorkbenchToolUiSession() override = default;
    virtual QString toolId() const = 0;
    virtual QString title() const = 0;
    virtual QWidget* surface() const = 0;
    virtual QString statusSummary() const = 0;
};

// Built-in composition root. WorkbenchWindow receives a homogeneous session list and
// never constructs mode panels or reaches into their binding controls.
[[nodiscard]] MISKEYED_WORKBENCH_SLANG_RHI_EXPORT QList<WorkbenchToolUiSession*>
createBuiltinToolUiSessions(QWidget* parent, SlangRhiWidget* sceneViewport,
    SlangRhiWidget* postViewport, SlangRhiWidget* shaderToyViewport, ShaderWorkspace* workspace,
    RenderToySession* renderToy, ShaderToySession* shaderToy);
} // namespace miskeyed::workbench::slang_rhi
