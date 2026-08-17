#include <slang_qrhi/WorkbenchWindow.h>
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("slang-qrhi"));
    const QString shader = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    slang_qrhi::WorkbenchWindow window(shader);
    window.show();
    return app.exec();
}
