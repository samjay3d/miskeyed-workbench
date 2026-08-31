#include <miskeyed/workbench/core/TimeContext.h>

#include <cmath>

namespace miskeyed::workbench::core {
namespace {
    double validRate(double rate)
    {
        return std::isfinite(rate) && rate > 0.0 ? rate : 24.0;
    }
}

TimeValue::TimeValue(double coordinateValue, double unitsPerSecond)
    : value(std::isfinite(coordinateValue) ? coordinateValue : 0.0)
    , rate(validRate(unitsPerSecond))
{
}

double TimeValue::seconds() const
{
    return value / validRate(rate);
}

TimeRange::TimeRange(TimeValue startValue, TimeValue durationValue)
    : start(startValue)
    , duration(durationValue)
{
}

TimeValue TimeRange::endTime() const
{
    return TimeValue(start.value + duration.seconds() * start.rate, start.rate);
}

TimeContext::TimeContext(QObject* parent)
    : QObject(parent)
{
}

void TimeContext::setSample(TimeValue current, TimeValue delta, quint64 evaluationIndex)
{
    current = TimeValue(current.value, current.rate);
    delta = TimeValue(delta.value, delta.rate);
    if (m_current.value == current.value && m_current.rate == current.rate
        && m_delta.value == delta.value && m_delta.rate == delta.rate
        && m_evaluationIndex == evaluationIndex)
        return;
    m_current = current;
    m_delta = delta;
    m_evaluationIndex = evaluationIndex;
    emit changed();
}

} // namespace miskeyed::workbench::core
