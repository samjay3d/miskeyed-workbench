#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>

#include <QApplication>
#include <QFile>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTimer>

using namespace miskeyed::workbench;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("miskeyed-workbench-wasm-spike"));

    QFile sample(QStringLiteral(":/miskeyed/workbench/browser/default.slang"));
    if (!sample.open(QIODevice::ReadOnly))
        return 2;

    slang_rhi::ShaderWorkspace workspace;
    auto* document = workspace.openSource(QUrl(QStringLiteral("workbench:/default.slang")),
        QStringLiteral("default.slang"), QString::fromUtf8(sample.readAll()));

    core::TimeContext time;
    core::TimeTransport transport;
    transport.setContext(&time);

    QMainWindow window;
    auto* split = new QSplitter(&window);
    auto* source = new QPlainTextEdit(document->source(), split);
    auto* viewport = new slang_rhi::SlangRhiWidget(split);
    viewport->setDocument(document);
    viewport->setTimeContext(&time);
    split->addWidget(source);
    split->addWidget(viewport);
    window.setCentralWidget(split);
    window.resize(1100, 650);
    window.setWindowTitle(QStringLiteral("Workbench WebAssembly Spike"));

    QObject::connect(source, &QPlainTextEdit::textChanged, document,
        [source, document] { document->setSource(source->toPlainText()); });
    document->compile();
    transport.setPlaying(true);
    QTimer clock;
    clock.setInterval(16);
    QObject::connect(&clock, &QTimer::timeout, &transport,
        [&transport] { transport.advanceSeconds(0.016); });
    clock.start();
    window.show();
    return app.exec();
}
