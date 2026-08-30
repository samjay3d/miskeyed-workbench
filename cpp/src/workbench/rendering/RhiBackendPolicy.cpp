#include <miskeyed/workbench/rendering/RhiBackendPolicy.h>
#include <QByteArray>
#include <QtGlobal>

namespace miskeyed::workbench::rendering {
namespace {
    RhiBackend g_default = RhiBackend::Auto;
}

RhiBackend RhiBackendPolicy::parse(QStringView value, bool* ok)
{
    const QString normalized = value.trimmed().toString().toLower();
    if (ok)
        *ok = true;
    if (normalized.isEmpty() || normalized == QStringLiteral("auto"))
        return RhiBackend::Auto;
    if (normalized == QStringLiteral("d3d") || normalized == QStringLiteral("d3d11")
        || normalized == QStringLiteral("direct3d11"))
        return RhiBackend::D3D11;
    if (normalized == QStringLiteral("vulkan"))
        return RhiBackend::Vulkan;
    if (normalized == QStringLiteral("metal"))
        return RhiBackend::Metal;
    if (ok)
        *ok = false;
    return RhiBackend::Auto;
}

QString RhiBackendPolicy::name(RhiBackend backend)
{
    switch (backend) {
    case RhiBackend::D3D11:
        return QStringLiteral("D3D11");
    case RhiBackend::Vulkan:
        return QStringLiteral("Vulkan");
    case RhiBackend::Metal:
        return QStringLiteral("Metal");
    case RhiBackend::Auto:
        return QStringLiteral("Auto");
    }
    return QStringLiteral("Auto");
}

RhiBackend RhiBackendPolicy::platformDefault()
{
#if defined(Q_OS_WIN)
    return RhiBackend::D3D11;
#elif defined(Q_OS_MACOS)
    return RhiBackend::Metal;
#else
    return RhiBackend::Vulkan;
#endif
}

bool RhiBackendPolicy::isSupportedOnHost(RhiBackend backend)
{
    if (backend == RhiBackend::Auto)
        return true;
#if defined(Q_OS_WIN)
    return backend == RhiBackend::D3D11 || backend == RhiBackend::Vulkan;
#elif defined(Q_OS_MACOS)
    return backend == RhiBackend::Metal;
#else
    return backend == RhiBackend::Vulkan;
#endif
}

QStringList RhiBackendPolicy::supportedNames()
{
    QStringList result { name(RhiBackend::Auto) };
#if defined(Q_OS_WIN)
    result << name(RhiBackend::D3D11) << name(RhiBackend::Vulkan);
#elif defined(Q_OS_MACOS)
    result << name(RhiBackend::Metal);
#else
    result << name(RhiBackend::Vulkan);
#endif
    return result;
}

QRhiWidget::Api RhiBackendPolicy::api(RhiBackend backend)
{
    if (backend == RhiBackend::Auto)
        backend = platformDefault();
    switch (backend) {
    case RhiBackend::D3D11:
        return QRhiWidget::Api::Direct3D11;
    case RhiBackend::Metal:
        return QRhiWidget::Api::Metal;
    case RhiBackend::Vulkan:
        return QRhiWidget::Api::Vulkan;
    case RhiBackend::Auto:
        break;
    }
    return QRhiWidget::Api::Vulkan;
}

RhiBackend RhiBackendPolicy::defaultBackend()
{
    if (g_default != RhiBackend::Auto)
        return g_default;
    bool ok = false;
    const RhiBackend environment = parse(qEnvironmentVariable("MISKEYED_WORKBENCH_RHI"), &ok);
    return ok && isSupportedOnHost(environment) ? environment : RhiBackend::Auto;
}

void RhiBackendPolicy::setDefaultBackend(RhiBackend backend)
{
    g_default = backend;
}

} // namespace miskeyed::workbench::rendering
