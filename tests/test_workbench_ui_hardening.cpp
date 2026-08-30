#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <miskeyed/workbench/slang/ShaderParameterModel.h>
#include <miskeyed/workbench/ui/ParameterInspector.h>
#include <miskeyed/workbench/ui/TimelineWidget.h>
#include <miskeyed/workbench/ui/ToolViewSelector.h>
#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <cassert>

using namespace miskeyed::workbench;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    slang_rhi::ShaderParameterModel parameters;
    QList<slang_rhi::ParameterDescriptor> descriptors;
    for (int i = 0; i < 48; ++i) {
        slang_rhi::ParameterDescriptor descriptor;
        descriptor.name = QStringLiteral("parameter%1").arg(i);
        descriptor.label = QStringLiteral("Parameter %1").arg(i);
        descriptor.group = QStringLiteral("Large reflected interface");
        descriptor.type = slang_rhi::ParameterType::Float;
        descriptor.offset = i * 4;
        descriptor.size = 4;
        descriptor.defaultValue = double(i);
        descriptors.push_back(descriptor);
    }
    parameters.setDescriptors(descriptors, descriptors.size() * 4);
    slang_rhi::ParameterInspector inspector;
    inspector.resize(300, 320);
    inspector.setModel(&parameters);
    inspector.show();
    app.processEvents();
    auto* scroll = inspector.findChild<QScrollArea*>(QStringLiteral("ParameterScrollArea"));
    assert(scroll);
    assert(scroll->verticalScrollBar()->maximum() > 0);

    ui::ToolViewSelector selector;
    for (int i = 0; i < 24; ++i)
        selector.addView(QStringLiteral("tool-%1").arg(i), QStringLiteral("Studio View %1").arg(i));
    assert(selector.viewCount() == 24);
    assert(selector.findChild<QComboBox*>()->count() == 24);

    core::TimeContext context;
    core::TimeTransport transport;
    transport.setContext(&context);
    ui::TimelineWidget timeline;
    timeline.setTimeModel(&context, &transport);
    QLineEdit focusOwner;
    focusOwner.show();
    focusOwner.setFocus();
    app.processEvents();
    assert(QApplication::focusWidget() == &focusOwner);
    transport.setPlaying(true);
    QEventLoop playbackLoop;
    QTimer::singleShot(120, &playbackLoop, &QEventLoop::quit);
    playbackLoop.exec();
    transport.setPlaying(false);
    assert(context.timeValue() > 0.0);
    assert(QApplication::focusWidget() == &focusOwner);
    return 0;
}
