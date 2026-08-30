#pragma once

#include <miskeyed/workbench/Export.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <QObject>

namespace miskeyed::workbench::core {

enum class LoopMode : quint8 { Clamp, Loop };

// Small playback controller over a TimeContext. It owns policy, never the evaluated
// resources, and can be replaced by USD, OTIO, offline-render, or external-host drivers.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT TimeTransport final : public QObject {
    Q_OBJECT
    Q_PROPERTY(TimeContext* context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(double rate READ rate WRITE setRate NOTIFY playbackChanged)
    Q_PROPERTY(double startValue READ startValue NOTIFY playbackChanged)
    Q_PROPERTY(double endValue READ endValue NOTIFY playbackChanged)
public:
    explicit TimeTransport(QObject* parent = nullptr);

    TimeContext* context() const { return m_context; }
    void setContext(TimeContext* context);
    TimeRange playbackRange() const { return m_playbackRange; }
    void setPlaybackRange(TimeRange range);
    void setPlaybackRange(TimeValue start, TimeValue end);
    bool playing() const { return m_playing; }
    void setPlaying(bool playing);
    LoopMode loopMode() const { return m_loopMode; }
    void setLoopMode(LoopMode mode) { m_loopMode = mode; }
    double rate() const { return m_playbackRange.start.rate; }
    double startValue() const { return m_playbackRange.start.value; }
    double endValue() const { return m_playbackRange.endTime().value; }

    void setRate(double rate);
    void seek(TimeValue value);
    void step(double units = 1.0);

signals:
    void contextChanged();
    void playbackChanged();
    void playingChanged(bool playing);

private:
    void evaluate(TimeValue value);

    TimeContext* m_context = nullptr;
    TimeRange m_playbackRange;
    bool m_playing = false;
    LoopMode m_loopMode = LoopMode::Loop;
    quint64 m_evaluationIndex = 0;
};

} // namespace miskeyed::workbench::core
