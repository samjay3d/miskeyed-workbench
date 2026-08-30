#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace miskeyed::workbench::core {

struct WorkbenchHeader final {
    QString id;
    QString title;
    QByteArray source;
};

// Returns the authored Slang contracts embedded from shaders/workbench. Compiler and UI
// consume this same catalog so the source shown to users cannot drift from compilation.
[[nodiscard]] QList<WorkbenchHeader> workbenchHeaders();
[[nodiscard]] QByteArray workbenchHeaderSource(const QString& id);

} // namespace miskeyed::workbench::core
