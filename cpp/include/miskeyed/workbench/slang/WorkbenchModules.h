#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace miskeyed::workbench::slang_rhi {

enum class WorkbenchModuleKind { Library, HostContract };

struct WorkbenchModule final {
    QString id;
    QString title;
    QString moduleName;
    QString path;
    QStringList imports;
    QByteArray source;
    WorkbenchModuleKind kind = WorkbenchModuleKind::Library;
    QString capabilityId;
    QString description;
    QString provides;
    QString hostProvider;
    QString consumer;
};

struct WorkbenchModuleState final {
    WorkbenchModule module;
    bool available = false;
    bool imported = false;
};

// Returns authored Slang contracts embedded from shaders/workbench. Module registration
// and built-in samples consume this packaging catalog at the Slang/mode edges.
[[nodiscard]] QList<WorkbenchModule> workbenchModules();
[[nodiscard]] QByteArray workbenchModuleSource(const QString& id);
// Projects compiler-resolved dependency identities onto the packaging catalog for
// discovery only. Slang remains the sole module resolver.
[[nodiscard]] QList<WorkbenchModuleState> workbenchModuleStates(
    const QStringList& resolvedDependencyIdentities);

} // namespace miskeyed::workbench::slang_rhi
