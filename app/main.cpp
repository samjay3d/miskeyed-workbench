#include <miskeyed/workbench/modes/render_toy/WorkbenchWindow.h>
#include <miskeyed/workbench/rendering/RhiBackendPolicy.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QTimer>
#include <cstdlib>
#include <cstdio>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("miskeyed-workbench"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Native Slang shader workbench"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("shader"), QStringLiteral("Shader to open."));
    parser.addOption({ QStringLiteral("rhi"),
        QStringLiteral("Presentation backend: auto, d3d11, vulkan, or metal."),
        QStringLiteral("backend"), QStringLiteral("auto") });
    parser.addOption({ QStringLiteral("rhi-smoke-test"),
        QStringLiteral("Exit successfully after QRhi initializes; fail after 30 seconds.") });
    parser.process(app);
    bool validBackend = false;
    const auto backend = miskeyed::workbench::rendering::RhiBackendPolicy::parse(
        parser.value(QStringLiteral("rhi")), &validBackend);
    if (!validBackend
        || !miskeyed::workbench::rendering::RhiBackendPolicy::isSupportedOnHost(backend)) {
        QTextStream(stderr)
            << "Unsupported --rhi value for this platform. Available: "
            << miskeyed::workbench::rendering::RhiBackendPolicy::supportedNames().join(", ")
            << '\n';
        return 2;
    }
    miskeyed::workbench::rendering::RhiBackendPolicy::setDefaultBackend(backend);
    const QString shader = parser.positionalArguments().value(0);
    miskeyed::workbench::slang_rhi::WorkbenchWindow window(shader);
    if (parser.isSet(QStringLiteral("rhi-smoke-test"))) {
        // Smoke the primary viewport of the initially visible Render Toy layout. QObject
        // child order is not a visibility contract: after adding ShaderToy, findChildren()
        // could return its hidden stacked-page viewport first and wait forever for a frame.
        auto* viewport = window.sceneViewport();
        if (!viewport) {
            QTextStream(stderr) << "RHI_SMOKE_FAIL no viewport\n";
            return 3;
        }
        QObject::connect(viewport,
            &miskeyed::workbench::slang_rhi::SlangRhiWidget::backendInitialized, &app,
            [](const QString& active, const QString& gpu) {
                QTextStream(stdout)
                    << "RHI_SMOKE_INITIALIZED backend=" << active << " gpu=" << gpu << '\n';
                std::fflush(nullptr);
            });
        QObject::connect(viewport, &miskeyed::workbench::slang_rhi::SlangRhiWidget::frameRendered,
            &app, [&app](const QString& active, const QString& gpu) {
                QTextStream(stdout) << "RHI_SMOKE_OK backend=" << active << " gpu=" << gpu << '\n';
                // This mode has proved device creation, pipeline creation, and draw
                // recording. Avoid normal UI/LSP teardown from inside render's callback.
                std::fflush(nullptr);
                std::_Exit(0);
            });
        QObject::connect(viewport, &miskeyed::workbench::slang_rhi::SlangRhiWidget::gpuError, &app,
            [&app](const QString& message) {
                QTextStream(stderr) << "RHI_SMOKE_FAIL " << message << '\n';
                app.exit(4);
            });
        QTimer::singleShot(30000, &app, [&app] {
            QTextStream(stderr) << "RHI_SMOKE_FAIL initialization timeout\n";
            app.exit(5);
        });
        viewport->update();
    }
    window.show();
    return app.exec();
}
