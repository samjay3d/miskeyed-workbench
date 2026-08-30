#pragma once

#include <miskeyed/workbench/Export.h>
#include <QWidget>

class QElapsedTimer;
class QTimer;

namespace miskeyed::workbench::core {
class TimeContext;
class TimeTransport;
}

namespace miskeyed::workbench::ui {

// Shared temporal presentation. Time state and playback policy remain owned by the
// workspace's TimeContext and TimeTransport; this widget owns only controls and a clock.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT TimelineWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget() override;

    void setTimeModel(core::TimeContext* context, core::TimeTransport* transport);
    core::TimeContext* timeContext() const { return m_context; }
    core::TimeTransport* timeTransport() const { return m_transport; }

private:
    void rebuildConnections();

    core::TimeContext* m_context = nullptr;
    core::TimeTransport* m_transport = nullptr;
    QTimer* m_timer = nullptr;
    QElapsedTimer* m_clock = nullptr;
};

} // namespace miskeyed::workbench::ui
