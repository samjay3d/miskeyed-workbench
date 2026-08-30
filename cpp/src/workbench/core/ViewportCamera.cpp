#include <miskeyed/workbench/core/ViewportCamera.h>
#include <miskeyed/workbench/core/WorkbenchHeaders.h>

#include <array>

namespace miskeyed::workbench::core {

QByteArray ViewportCameraBinding::slangDeclaration()
{
    return workbenchHeaderSource(QStringLiteral("viewport-camera"));
}

bool ViewportCameraBinding::isCameraParameter(const QString& name)
{
    constexpr std::array names { yaw, pitch, distance, fov, panX, panY };
    for (const char* candidate : names) {
        if (name == QLatin1String(candidate))
            return true;
    }
    return false;
}

} // namespace miskeyed::workbench::core
