#include <miskeyed/workbench/slang/WorkbenchModules.h>

#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

int main()
{
    const auto states
        = workbenchModuleStates({ QStringLiteral("miskeyed.time"), QStringLiteral("miskeyed.ui") });
    auto find = [&states](const QString& name) -> const WorkbenchModuleState* {
        for (const WorkbenchModuleState& state : states)
            if (state.module.moduleName == name)
                return &state;
        return nullptr;
    };

    const auto* time = find(QStringLiteral("miskeyed.time"));
    assert(time && time->module.kind == WorkbenchModuleKind::HostContract);
    assert(time->module.capabilityId == QStringLiteral("time"));
    assert(time->available && time->imported);

    const auto* ui = find(QStringLiteral("miskeyed.ui"));
    assert(ui && ui->module.kind == WorkbenchModuleKind::Library);
    assert(ui->module.capabilityId.isEmpty());
    assert(ui->available && ui->imported);

    const auto* renderToy = find(QStringLiteral("miskeyed.render_toy"));
    assert(renderToy && renderToy->available && !renderToy->imported);
    return 0;
}
