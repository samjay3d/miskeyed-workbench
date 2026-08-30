#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>
#include <QtTypes>

namespace miskeyed::workbench::core {

// A temporal coordinate and the number of coordinate units per second. The value is
// deliberately floating point: subframes and externally-authored time codes are valid.
struct MISKEYED_WORKBENCH_SLANG_RHI_EXPORT TimeValue final {
    double value = 0.0;
    double rate = 24.0;

    TimeValue() = default;
    TimeValue(double value, double rate);
    [[nodiscard]] double seconds() const;
};

// Descriptive temporal domain only. It never clamps evaluation samples.
struct MISKEYED_WORKBENCH_SLANG_RHI_EXPORT TimeRange final {
    TimeValue start;
    TimeValue duration { 240.0, 24.0 };

    TimeRange() = default;
    TimeRange(TimeValue start, TimeValue duration);
    [[nodiscard]] TimeValue endTime() const;
};

// The sample currently being evaluated. Playback range, looping, stepping, and clock
// policy live in a controller such as TimeTransport, not in this context.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT TimeContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(double timeValue READ timeValue NOTIFY changed)
    Q_PROPERTY(double timeRate READ timeRate NOTIFY changed)
    Q_PROPERTY(double timeSeconds READ timeSeconds NOTIFY changed)
    Q_PROPERTY(double deltaValue READ deltaValue NOTIFY changed)
    Q_PROPERTY(double deltaRate READ deltaRate NOTIFY changed)
    Q_PROPERTY(double deltaSeconds READ deltaSeconds NOTIFY changed)
    Q_PROPERTY(quint64 evaluationIndex READ evaluationIndex NOTIFY changed)
public:
    explicit TimeContext(QObject* parent = nullptr);

    TimeValue current() const { return m_current; }
    TimeValue delta() const { return m_delta; }
    double timeValue() const { return m_current.value; }
    double timeRate() const { return m_current.rate; }
    double timeSeconds() const { return m_current.seconds(); }
    double deltaValue() const { return m_delta.value; }
    double deltaRate() const { return m_delta.rate; }
    double deltaSeconds() const { return m_delta.seconds(); }
    quint64 evaluationIndex() const { return m_evaluationIndex; }

    void setSample(TimeValue current, TimeValue delta, quint64 evaluationIndex);

signals:
    void changed();

private:
    TimeValue m_current;
    TimeValue m_delta { 1.0, 24.0 };
    quint64 m_evaluationIndex = 0;
};

// Names in the Workbench-owned shader contract. This simplified GPU ABI is an adapter
// over TimeContext; it is not the core temporal representation.
struct TimeBinding final {
    static constexpr const char* time = "workbenchTime.time";
    static constexpr const char* deltaTime = "workbenchTime.deltaTime";
    static constexpr const char* frame = "workbenchTime.frame";
    static constexpr const char* frameRate = "workbenchTime.frameRate";
};

} // namespace miskeyed::workbench::core
