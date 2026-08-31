#include <miskeyed/workbench/rendering/RhiBackendPolicy.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <QApplication>
#include <cassert>

using namespace miskeyed::workbench::rendering;

using miskeyed::workbench::slang_rhi::SlangRhiWidget;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    bool ok = false;
    assert(RhiBackendPolicy::parse(u"vulkan", &ok) == RhiBackend::Vulkan && ok);
    assert(RhiBackendPolicy::parse(u"D3D", &ok) == RhiBackend::D3D11 && ok);
    assert(RhiBackendPolicy::parse(u"unknown", &ok) == RhiBackend::Auto && !ok);
    assert(RhiBackendPolicy::isSupportedOnHost(RhiBackendPolicy::platformDefault()));
    assert(RhiBackendPolicy::api(RhiBackend::Auto)
        == RhiBackendPolicy::api(RhiBackendPolicy::platformDefault()));
    assert(RhiBackendPolicy::supportedNames().contains(QStringLiteral("Auto")));

    const auto hostPolicy = RhiBackendPolicy::platformDefault();
    SlangRhiWidget scene(hostPolicy);
    SlangRhiWidget post(hostPolicy);
    SlangRhiWidget shaderToy(hostPolicy);
    assert(scene.backend() == hostPolicy);
    assert(post.backend() == hostPolicy);
    assert(shaderToy.backend() == hostPolicy);
    assert(scene.api() == post.api() && post.api() == shaderToy.api());
    return 0;
}
