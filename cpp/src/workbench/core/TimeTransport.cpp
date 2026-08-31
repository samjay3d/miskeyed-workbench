#include <miskeyed/workbench/core/TimeTransport.h>

#include <algorithm>
#include <cmath>

namespace miskeyed::workbench::core {

TimeTransport::TimeTransport(QObject* parent)
    : QObject(parent)
{
}

void TimeTransport::setContext(TimeContext* context)
{
    if (m_context == context)
        return;
    m_context = context;
    emit contextChanged();
    if (m_context)
        evaluate(m_context->current());
}

void TimeTransport::setPlaybackRange(TimeRange range)
{
    TimeValue start = range.start;
    TimeValue end = range.endTime();
    if (end.seconds() < start.seconds())
        std::swap(start, end);
    m_playbackRange
        = TimeRange(start, TimeValue((end.seconds() - start.seconds()) * start.rate, start.rate));
    emit playbackChanged();
}

void TimeTransport::setPlaybackRange(TimeValue start, TimeValue end)
{
    if (end.seconds() < start.seconds())
        std::swap(start, end);
    setPlaybackRange(
        TimeRange(start, TimeValue((end.seconds() - start.seconds()) * start.rate, start.rate)));
}

void TimeTransport::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged(playing);
}

void TimeTransport::setRate(double newRate)
{
    if (!std::isfinite(newRate) || newRate <= 0.0 || newRate == rate())
        return;
    const TimeValue start(m_playbackRange.start.seconds() * newRate, newRate);
    const TimeValue end(m_playbackRange.endTime().seconds() * newRate, newRate);
    const double currentSeconds = m_context ? m_context->current().seconds() : 0.0;
    setPlaybackRange(start, end);
    if (m_context)
        seek(TimeValue(currentSeconds * newRate, newRate));
}

void TimeTransport::seek(TimeValue value)
{
    evaluate(value);
}

void TimeTransport::step(double units)
{
    const TimeValue current = m_context ? m_context->current() : m_playbackRange.start;
    TimeValue next(current.value + units, current.rate);
    const TimeValue start(m_playbackRange.start.value, current.rate);
    const TimeValue end((m_playbackRange.endTime().seconds()) * current.rate, current.rate);
    if (next.value > end.value)
        next.value = m_loopMode == LoopMode::Loop ? start.value : end.value;
    else if (next.value < start.value)
        next.value = m_loopMode == LoopMode::Loop ? end.value : start.value;
    evaluate(next);
}

void TimeTransport::advanceSeconds(double elapsedSeconds)
{
    if (!m_context || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0)
        return;
    const double start = m_playbackRange.start.seconds();
    const double end = m_playbackRange.endTime().seconds();
    double next = m_context->current().seconds() + elapsedSeconds;
    if (next < start) {
        next = m_loopMode == LoopMode::Loop ? end : start;
    } else if (next > end) {
        if (m_loopMode == LoopMode::Loop && end > start)
            next = start + std::fmod(next - start, end - start);
        else
            next = end;
    }
    const double sampleRate = m_context->current().rate;
    evaluate(TimeValue(next * sampleRate, sampleRate),
        TimeValue(elapsedSeconds * sampleRate, sampleRate));
}

void TimeTransport::evaluate(TimeValue value)
{
    evaluate(value, TimeValue(1.0, value.rate));
}

void TimeTransport::evaluate(TimeValue value, TimeValue delta)
{
    if (!m_context)
        return;
    ++m_evaluationIndex;
    m_context->setSample(value, delta, m_evaluationIndex);
}

} // namespace miskeyed::workbench::core
