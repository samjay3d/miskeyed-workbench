#include <miskeyed/workbench/core/ViewportCamera.h>
#include <miskeyed/workbench/slang/WorkbenchModules.h>

int main()
{
    using miskeyed::workbench::core::ViewportCameraBinding;

    const QByteArray header
        = miskeyed::workbench::slang_rhi::workbenchModuleSource(QStringLiteral("viewport-camera"));
    const auto headers = miskeyed::workbench::slang_rhi::workbenchModules();
    const bool valid = headers.size() >= 4 && !headers.at(0).source.isEmpty()
        && header
            == miskeyed::workbench::slang_rhi::workbenchModuleSource(
                QStringLiteral("viewport-camera"))
        && header.contains("struct ViewportCamera")
        && header.contains("uniform ViewportCamera camera")
        && header.contains(ViewportCameraBinding::yaw)
        && header.contains(ViewportCameraBinding::distance)
        && ViewportCameraBinding::isCameraParameter(QStringLiteral("camFov"))
        && !ViewportCameraBinding::isCameraParameter(QStringLiteral("cameraExposure"));
    return valid ? 0 : 1;
}
