#include <miskeyed/workbench/rendering/RhiBackendPolicy.h>
#include <cassert>

using namespace miskeyed::workbench::rendering;

int main()
{
    bool ok = false;
    assert(RhiBackendPolicy::parse(u"vulkan", &ok) == RhiBackend::Vulkan && ok);
    assert(RhiBackendPolicy::parse(u"D3D", &ok) == RhiBackend::D3D11 && ok);
    assert(RhiBackendPolicy::parse(u"unknown", &ok) == RhiBackend::Auto && !ok);
    assert(RhiBackendPolicy::isSupportedOnHost(RhiBackendPolicy::platformDefault()));
    assert(RhiBackendPolicy::api(RhiBackend::Auto)
        == RhiBackendPolicy::api(RhiBackendPolicy::platformDefault()));
    assert(RhiBackendPolicy::supportedNames().contains(QStringLiteral("Auto")));
    return 0;
}
