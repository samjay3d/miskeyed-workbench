#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/WorkbenchHeaders.h>
#include <QByteArray>
#include <algorithm>

namespace miskeyed::workbench::core {

TimeContext::TimeContext(QObject* parent)
    : QObject(parent)
{
}

void TimeContext::updateSample()
{
    m_timeSeconds = double(m_frame) / m_frameRate;
    m_deltaSeconds = 1.0 / m_frameRate;
    emit changed();
}

void TimeContext::setFrame(qint64 frame)
{
    frame = std::clamp(frame, m_startFrame, m_endFrame);
    if (m_frame == frame)
        return;
    m_frame = frame;
    updateSample();
}

void TimeContext::setFrameRate(double frameRate)
{
    frameRate = std::max(0.001, frameRate);
    if (qFuzzyCompare(m_frameRate, frameRate))
        return;
    m_frameRate = frameRate;
    updateSample();
}

void TimeContext::setRange(qint64 startFrame, qint64 endFrame)
{
    if (startFrame > endFrame)
        std::swap(startFrame, endFrame);
    if (m_startFrame == startFrame && m_endFrame == endFrame)
        return;
    m_startFrame = startFrame;
    m_endFrame = endFrame;
    m_frame = std::clamp(m_frame, m_startFrame, m_endFrame);
    updateSample();
}

void TimeContext::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged(playing);
}

void TimeContext::step(qint64 frames)
{
    qint64 next = m_frame + frames;
    if (m_playing && next > m_endFrame)
        next = m_startFrame;
    setFrame(next);
}

QByteArray TimeBinding::slangDeclaration()
{
    return workbenchHeaderSource(QStringLiteral("time"));
}

} // namespace miskeyed::workbench::core
