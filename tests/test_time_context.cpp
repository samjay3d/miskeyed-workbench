#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <QCoreApplication>
#include <cassert>
#include <cmath>

using miskeyed::workbench::core::TimeBinding;
using miskeyed::workbench::core::TimeContext;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    TimeContext time;
    time.setFrameRate(24.0);
    time.setFrame(120);
    assert(time.frame() == 120);
    assert(std::abs(time.timeSeconds() - 5.0) < 1.0e-12);
    assert(std::abs(time.deltaSeconds() - 1.0 / 24.0) < 1.0e-12);

    time.setFrame(0);
    time.step(1);
    assert(time.frame() == 1);
    assert(std::abs(time.timeSeconds() - 1.0 / 24.0) < 1.0e-12);

    const QByteArray contract
        = miskeyed::workbench::slang_rhi::workbenchModuleSource(QStringLiteral("time"));
    assert(contract.contains("WorkbenchTime"));
    assert(contract.contains("workbenchTime"));
    assert(contract.contains("deltaTime"));
    return 0;
}
