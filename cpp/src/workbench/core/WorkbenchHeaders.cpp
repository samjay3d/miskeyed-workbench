#include <miskeyed/workbench/core/WorkbenchHeaders.h>

#include <QFile>

static void initializeWorkbenchHeaderResources()
{
    Q_INIT_RESOURCE(workbench_headers);
}

namespace miskeyed::workbench::core {
namespace {

    QByteArray readHeader(const char* path)
    {
        static const bool initialized = [] {
            initializeWorkbenchHeaderResources();
            return true;
        }();
        Q_UNUSED(initialized);
        QFile file(QString::fromLatin1(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray {};
    }

} // namespace

QList<WorkbenchHeader> workbenchHeaders()
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

QByteArray workbenchHeaderSource(const QString& id)
{
    for (const WorkbenchHeader& header : workbenchHeaders()) {
        if (header.id == id)
            return header.source;
    }
    return {};
}

} // namespace miskeyed::workbench::core
