#include <miskeyed/workbench/slang/WorkbenchModules.h>

#include <QFile>

static void initializeWorkbenchModuleResources()
{
    Q_INIT_RESOURCE(workbench_headers);
}

namespace miskeyed::workbench::slang_rhi {
namespace {

    QByteArray readHeader(const char* path)
    {
        static const bool initialized = [] {
            initializeWorkbenchModuleResources();
            return true;
        }();
        Q_UNUSED(initialized);
        QFile file(QString::fromLatin1(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray {};
    }

} // namespace

QList<WorkbenchModule> workbenchModules()
{
    return {
        { QStringLiteral("ui"), QStringLiteral("UI attributes"), QStringLiteral("miskeyed.ui"),
            QStringLiteral("miskeyed/ui.slang"), {},
            readHeader(":/miskeyed/workbench/headers/ui.slang") },
        { QStringLiteral("viewport-camera"), QStringLiteral("Viewport camera"),
            QStringLiteral("miskeyed.viewport_camera"),
            QStringLiteral("miskeyed/viewport_camera.slang"), { QStringLiteral("miskeyed.ui") },
            readHeader(":/miskeyed/workbench/headers/viewport_camera.slang") },
        { QStringLiteral("time"), QStringLiteral("Time evaluation"),
            QStringLiteral("miskeyed.time"), QStringLiteral("miskeyed/time.slang"), {},
            readHeader(":/miskeyed/workbench/headers/time.slang") },
        { QStringLiteral("render-toy"), QStringLiteral("Render Toy pass"),
            QStringLiteral("miskeyed.render_toy"), QStringLiteral("miskeyed/render_toy.slang"), {},
            readHeader(":/miskeyed/workbench/headers/render_toy.slang") },
    };
}

QByteArray workbenchModuleSource(const QString& id)
{
    for (const WorkbenchModule& header : workbenchModules()) {
        if (header.id == id)
            return header.source;
    }
    return {};
}

} // namespace miskeyed::workbench::slang_rhi
