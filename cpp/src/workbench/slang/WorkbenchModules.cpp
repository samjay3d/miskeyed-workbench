#include <miskeyed/workbench/slang/WorkbenchModules.h>

#include <QFile>

static void initializeWorkbenchModuleResources()
{
    Q_INIT_RESOURCE(workbench_headers);
}

namespace miskeyed::workbench::slang_rhi {
namespace {

    QByteArray readHeader(const char* path)
    {
        static const bool initialized = [] {
            initializeWorkbenchModuleResources();
            return true;
        }();
        Q_UNUSED(initialized);
        QFile file(QString::fromLatin1(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray {};
    }

} // namespace

QList<WorkbenchModule> workbenchModules()
{
    return {
        { QStringLiteral("ui"), QStringLiteral("UI attributes"), QStringLiteral("miskeyed.ui"),
            QStringLiteral("miskeyed/ui.slang"), {},
            readHeader(":/miskeyed/workbench/headers/ui.slang"), WorkbenchModuleKind::Library, {},
            QStringLiteral("Semantic annotations for reflected controls."),
            QStringLiteral("UIName, UIRange, UIStep, UIWidget, UIGroup, UITooltip, UIUnits"),
            QStringLiteral("Packaged Slang library"), QStringLiteral("Inspector reflection") },
        { QStringLiteral("viewport-camera"), QStringLiteral("Viewport camera"),
            QStringLiteral("miskeyed.viewport_camera"),
            QStringLiteral("miskeyed/viewport_camera.slang"), { QStringLiteral("miskeyed.ui") },
            readHeader(":/miskeyed/workbench/headers/viewport_camera.slang"),
            WorkbenchModuleKind::HostContract, QStringLiteral("viewport-camera"),
            QStringLiteral("Viewport camera state exposed as reflected shader parameters."),
            QStringLiteral("camera yaw, pitch, distance, FOV, and pan"),
            QStringLiteral("Workbench viewport camera"), QStringLiteral("Render Toy viewports") },
        { QStringLiteral("time"), QStringLiteral("Time evaluation"),
            QStringLiteral("miskeyed.time"), QStringLiteral("miskeyed/time.slang"), {},
            readHeader(":/miskeyed/workbench/headers/time.slang"),
            WorkbenchModuleKind::HostContract, QStringLiteral("time"),
            QStringLiteral("Deterministic workspace evaluation state."),
            QStringLiteral("time, deltaTime, frame, frameRate"),
            QStringLiteral("Workspace TimeContext"),
            QStringLiteral("Render Toy and Shader Toy render surfaces") },
        { QStringLiteral("render-toy"), QStringLiteral("Render Toy pass"),
            QStringLiteral("miskeyed.render_toy"), QStringLiteral("miskeyed/render_toy.slang"), {},
            readHeader(":/miskeyed/workbench/headers/render_toy.slang"),
            WorkbenchModuleKind::HostContract, QStringLiteral("render-toy"),
            QStringLiteral("Scene and post-process pass interface."),
            QStringLiteral("SceneSample, sceneColor, sampleScene"),
            QStringLiteral("Render Toy two-pass renderer"), QStringLiteral("RenderToySession") },
        { QStringLiteral("shader-toy"), QStringLiteral("Shader Toy inputs"),
            QStringLiteral("miskeyed.shader_toy"), QStringLiteral("miskeyed/shader_toy.slang"), {},
            readHeader(":/miskeyed/workbench/headers/shader_toy.slang"),
            WorkbenchModuleKind::HostContract, QStringLiteral("shader-toy"),
            QStringLiteral("Fullscreen presentation inputs."),
            QStringLiteral("shaderToy.resolution"), QStringLiteral("Shader Toy render surface"),
            QStringLiteral("ShaderToySession") },
    };
}

QList<WorkbenchModuleState> workbenchModuleStates(const QStringList& resolvedDependencyIdentities)
{
    QList<WorkbenchModuleState> states;
    for (const WorkbenchModule& module : workbenchModules()) {
        states.push_back(
            { module, true, resolvedDependencyIdentities.contains(module.moduleName) });
    }
    return states;
}

QByteArray workbenchModuleSource(const QString& id)
{
    for (const WorkbenchModule& header : workbenchModules()) {
        if (header.id == id)
            return header.source;
    }
    return {};
}

} // namespace miskeyed::workbench::slang_rhi
