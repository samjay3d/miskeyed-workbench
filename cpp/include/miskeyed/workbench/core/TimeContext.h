#pragma once

#include <QObject>
#include <QtTypes>

namespace miskeyed::workbench::core {

// Deterministic evaluation state shared by every consumer participating in a frame.
// It contains no clock or UI policy: timelines, offline renderers, and external hosts
// are controllers that advance this same model.
class TimeContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(double timeSeconds READ timeSeconds NOTIFY changed)
    Q_PROPERTY(double deltaSeconds READ deltaSeconds NOTIFY changed)
    Q_PROPERTY(qint64 frame READ frame WRITE setFrame NOTIFY changed)
    Q_PROPERTY(double frameRate READ frameRate WRITE setFrameRate NOTIFY changed)
    Q_PROPERTY(qint64 startFrame READ startFrame NOTIFY changed)
    Q_PROPERTY(qint64 endFrame READ endFrame NOTIFY changed)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
public:
    explicit TimeContext(QObject* parent = nullptr);

    double timeSeconds() const { return m_timeSeconds; }
    double deltaSeconds() const { return m_deltaSeconds; }
    qint64 frame() const { return m_frame; }
    double frameRate() const { return m_frameRate; }
    qint64 startFrame() const { return m_startFrame; }
    qint64 endFrame() const { return m_endFrame; }
    bool playing() const { return m_playing; }

    void setFrame(qint64 frame);
    void setFrameRate(double frameRate);
    void setRange(qint64 startFrame, qint64 endFrame);
    void setPlaying(bool playing);
    void step(qint64 frames);

signals:
    void changed();
    void playingChanged(bool playing);

private:
    void updateSample();

    double m_timeSeconds = 0.0;
    double m_deltaSeconds = 1.0 / 24.0;
    qint64 m_frame = 0;
    double m_frameRate = 24.0;
    qint64 m_startFrame = 0;
    qint64 m_endFrame = 240;
    bool m_playing = false;
};

// Names in the Workbench-owned shader contract. The declaration is currently part
// of the small compiler contract; this boundary can be served by Slang's filesystem
// hook when embedded modules move behind ISlangFileSystem.
struct TimeBinding final {
    static constexpr const char* time = "workbenchTime.time";
    static constexpr const char* deltaTime = "workbenchTime.deltaTime";
    static constexpr const char* frame = "workbenchTime.frame";
    static constexpr const char* frameRate = "workbenchTime.frameRate";
};

} // namespace miskeyed::workbench::core
