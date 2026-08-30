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
        { QStringLiteral("ui"), QStringLiteral("UI attributes"),
            readHeader(":/miskeyed/workbench/headers/ui.slang") },
        { QStringLiteral("viewport-camera"), QStringLiteral("Viewport camera"),
            readHeader(":/miskeyed/workbench/headers/viewport_camera.slang") },
        { QStringLiteral("time"), QStringLiteral("Time evaluation"),
            readHeader(":/miskeyed/workbench/headers/time.slang") },
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
