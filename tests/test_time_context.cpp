#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <QCoreApplication>
#include <cassert>
#include <cmath>

using miskeyed::workbench::core::LoopMode;
using miskeyed::workbench::core::TimeContext;
using miskeyed::workbench::core::TimeRange;
using miskeyed::workbench::core::TimeTransport;
using miskeyed::workbench::core::TimeValue;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    assert(std::abs(TimeValue(24.0, 24.0).seconds() - 1.0) < 1.0e-12);
    assert(std::abs(TimeValue(48.0, 24.0).seconds() - 2.0) < 1.0e-12);
    assert(std::abs(TimeValue(24000.0, 24000.0).seconds() - 1.0) < 1.0e-12);
    assert(std::abs(TimeValue(23.5, 24.0).seconds() - 23.5 / 24.0) < 1.0e-12);

    const TimeRange range(TimeValue(10.0, 24.0), TimeValue(20.0, 24.0));
    assert(std::abs(range.endTime().value - 30.0) < 1.0e-12);

    TimeContext context;
    context.setSample(TimeValue(101.5, 24.0), TimeValue(0.25, 24.0), 7);
    assert(std::abs(context.current().value - 101.5) < 1.0e-12);
    assert(std::abs(context.timeSeconds() - 101.5 / 24.0) < 1.0e-12);
    assert(std::abs(context.deltaSeconds() - 0.25 / 24.0) < 1.0e-12);
    assert(context.evaluationIndex() == 7);
    // Evaluation state is not clamped by any playback range.
    context.setSample(TimeValue(1001.25, 24.0), TimeValue(1.0, 24.0), 8);
    assert(context.current().value == 1001.25);

    TimeTransport transport;
    transport.setContext(&context);
    transport.setPlaybackRange(TimeValue(0.0, 24.0), TimeValue(2.0, 24.0));
    transport.setLoopMode(LoopMode::Loop);
    transport.seek(TimeValue(2.0, 24.0));
    transport.step();
    assert(context.current().value == 0.0); // looping belongs to transport
    transport.seek(TimeValue(23.5, 24.0));
    assert(context.current().value == 23.5); // seek itself does not clamp evaluation

    transport.setPlaybackRange(TimeValue(12.5, 24.0), TimeValue(60.25, 24.0));
    transport.seek(TimeValue(24.0, 24.0));
    transport.setRate(48.0);
    assert(std::abs(context.current().value - 48.0) < 1.0e-12);
    assert(std::abs(context.timeSeconds() - 1.0) < 1.0e-12);
    assert(std::abs(transport.startValue() - 25.0) < 1.0e-12);
    assert(std::abs(transport.endValue() - 120.5) < 1.0e-12);
    transport.setRate(24.0);
    assert(std::abs(context.current().value - 24.0) < 1.0e-12);
    assert(std::abs(transport.startValue() - 12.5) < 1.0e-12);
    assert(std::abs(transport.endValue() - 60.25) < 1.0e-12);

    transport.seek(TimeValue(24.0, 24.0));
    transport.advanceSeconds(0.5);
    assert(std::abs(context.current().value - 36.0) < 1.0e-12);
    assert(std::abs(context.deltaSeconds() - 0.5) < 1.0e-12);

    transport.setLoopMode(LoopMode::Clamp);
    transport.seek(TimeValue(0.0, 24.0));
    transport.advanceSeconds(0.1);
    assert(std::abs(context.current().value - transport.startValue()) < 1.0e-12);
    transport.setLoopMode(LoopMode::Loop);
    transport.seek(TimeValue(0.0, 24.0));
    transport.advanceSeconds(0.1);
    assert(std::abs(context.current().value - transport.endValue()) < 1.0e-12);

    const QByteArray contract
        = miskeyed::workbench::slang_rhi::workbenchModuleSource(QStringLiteral("time"));
    assert(contract.contains("WorkbenchTime"));
    assert(contract.contains("workbenchTime"));
    assert(contract.contains("deltaTime"));
    return 0;
}
