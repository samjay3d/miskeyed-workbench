#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace miskeyed::workbench::slang_rhi {

struct WorkbenchModule final {
    QString id;
    QString title;
    QString moduleName;
    QString path;
    QStringList imports;
    QByteArray source;
};

// Returns authored Slang contracts embedded from shaders/workbench. Module registration
// and built-in samples consume this packaging catalog at the Slang/mode edges.
[[nodiscard]] QList<WorkbenchModule> workbenchModules();
[[nodiscard]] QByteArray workbenchModuleSource(const QString& id);

} // namespace miskeyed::workbench::slang_rhi
