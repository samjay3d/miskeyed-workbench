#include "SlangToolLocator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace miskeyed::workbench::platform {
namespace {

    QString executableName(QStringView baseName)
    {
#if defined(Q_OS_WIN)
        return baseName.toString() + QStringLiteral(".exe");
#else
        return baseName.toString();
#endif
    }

} // namespace

QString findSlangLanguageServer()
{
    const QString configured = qEnvironmentVariable("SLANGD_PATH");
    if (!configured.isEmpty() && QFileInfo::exists(configured))
        return configured;

    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("slangd"));
    if (!fromPath.isEmpty())
        return fromPath;

    const QString tool = executableName(u"slangd");
    QStringList candidates;
    const QString slangRoot = qEnvironmentVariable("SLANG_ROOT");
    if (!slangRoot.isEmpty())
        candidates << QDir(slangRoot).filePath(QStringLiteral("bin/") + tool);
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath(tool);
    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return candidate;
    }
    return {};
}

} // namespace miskeyed::workbench::platform
