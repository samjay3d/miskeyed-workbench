#include <miskeyed/workbench/slang_rhi/WorkbenchWindow.h>
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("miskeyed-workbench"));
    const QString shader = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    miskeyed::workbench::slang_rhi::WorkbenchWindow window(shader);
    window.show();
    return app.exec();
}
