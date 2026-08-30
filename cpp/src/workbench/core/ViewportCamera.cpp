#include <miskeyed/workbench/core/ViewportCamera.h>

#include <array>

namespace miskeyed::workbench::core {

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
