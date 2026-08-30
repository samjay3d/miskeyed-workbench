#pragma once

#include <QString>

namespace miskeyed::workbench::core {

// Host-side description of the camera contract exposed to authored shaders. The
// fields intentionally match the leaf names reflected from WorkbenchViewportCamera.
struct ViewportCamera final {
    float camYaw = 0.0f;
    float camPitch = 0.0f;
    float camDistance = 0.0f;
    float camFov = 0.0f;
    float camPanX = 0.0f;
    float camPanY = 0.0f;
};

struct ViewportCameraBinding final {
    static constexpr const char* yaw = "camYaw";
    static constexpr const char* pitch = "camPitch";
    static constexpr const char* distance = "camDistance";
    static constexpr const char* fov = "camFov";
    static constexpr const char* panX = "camPanX";
    static constexpr const char* panY = "camPanY";

    [[nodiscard]] static bool isCameraParameter(const QString& name);
};

} // namespace miskeyed::workbench::core
