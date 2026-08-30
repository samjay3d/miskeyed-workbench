#pragma once

#include <miskeyed/workbench/Export.h>
#include <QRhiWidget>
#include <QString>
#include <QStringList>

namespace miskeyed::workbench::rendering {

// Selection of the presentation API is a host policy. It must not leak into authored
// Slang or choose which generated-code tab the editor is inspecting.
enum class RhiBackend { Auto, D3D11, Vulkan, Metal };

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT RhiBackendPolicy final {
public:
    static RhiBackend parse(QStringView value, bool* ok = nullptr);
    static QString name(RhiBackend backend);
    static RhiBackend platformDefault();
    static bool isSupportedOnHost(RhiBackend backend);
    static QStringList supportedNames();
    static QRhiWidget::Api api(RhiBackend backend);

    static RhiBackend defaultBackend();
    static void setDefaultBackend(RhiBackend backend);
};

} // namespace miskeyed::workbench::rendering
