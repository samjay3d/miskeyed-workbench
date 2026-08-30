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
    const TimeValue start(m_playbackRange.start.value, newRate);
    const TimeValue end(endValue(), newRate);
    setPlaybackRange(start, end);
    if (m_context)
        seek(TimeValue(m_context->current().value, newRate));
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

void TimeTransport::evaluate(TimeValue value)
{
    if (!m_context)
        return;
    ++m_evaluationIndex;
    m_context->setSample(value, TimeValue(1.0, value.rate), m_evaluationIndex);
}

} // namespace miskeyed::workbench::core
