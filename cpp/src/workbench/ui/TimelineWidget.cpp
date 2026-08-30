#include <miskeyed/workbench/ui/TimelineWidget.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

namespace miskeyed::workbench::ui {

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
    , m_clock(new QElapsedTimer)
{
    setObjectName(QStringLiteral("Timeline"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

TimelineWidget::~TimelineWidget()
{
    delete m_clock;
}

void TimelineWidget::setTimeModel(core::TimeContext* context, core::TimeTransport* transport)
{
    if (m_context == context && m_transport == transport)
        return;
    if (m_context)
        disconnect(m_context, nullptr, this, nullptr);
    if (m_transport)
        disconnect(m_transport, nullptr, this, nullptr);
    m_context = context;
    m_transport = transport;
    rebuildConnections();
}

void TimelineWidget::rebuildConnections()
{
    disconnect(m_timer, nullptr, this, nullptr);
    m_timer->stop();
    if (layout())
        delete layout();
    const auto children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children)
        delete child;
    if (!m_context || !m_transport)
        return;

    auto* rows = new QVBoxLayout(this);
    rows->setContentsMargins(8, 4, 8, 5);
    rows->setSpacing(3);
    auto* transportRow = new QHBoxLayout;
    auto* rulerRow = new QHBoxLayout;
    rows->addLayout(transportRow);
    rows->addLayout(rulerRow);

    auto button = [this, transportRow](const QString& text, const QString& tip) {
        auto* result = new QPushButton(text, this);
        result->setObjectName(QStringLiteral("TransportButton"));
        result->setToolTip(tip);
        result->setFocusPolicy(Qt::NoFocus);
        result->setFixedWidth(32);
        transportRow->addWidget(result);
        return result;
    };
    auto* first = button(QStringLiteral("|<"), QStringLiteral("First frame"));
    auto* previous = button(QStringLiteral("<"), QStringLiteral("Previous frame"));
    auto* play = button(QStringLiteral("Play"), QStringLiteral("Play / pause"));
    play->setMinimumWidth(48);
    auto* next = button(QStringLiteral(">"), QStringLiteral("Next frame"));
    auto* last = button(QStringLiteral(">|"), QStringLiteral("Last frame"));
    auto* frameReadout = new QLabel(this);
    frameReadout->setObjectName(QStringLiteral("TimelineFrameReadout"));
    auto* timeReadout = new QLabel(this);
    timeReadout->setObjectName(QStringLiteral("TimelineTimeReadout"));
    transportRow->addSpacing(8);
    transportRow->addWidget(frameReadout);
    transportRow->addWidget(timeReadout);
    transportRow->addStretch(1);

    auto* start = new QDoubleSpinBox(this);
    auto* end = new QDoubleSpinBox(this);
    auto* rate = new QDoubleSpinBox(this);
    auto* scrubber = new QSlider(Qt::Horizontal, this);
    scrubber->setObjectName(QStringLiteral("TimelineScrubber"));
    scrubber->setFocusPolicy(Qt::ClickFocus);
    for (auto* spin : { start, end }) {
        spin->setRange(-1000000.0, 1000000.0);
        spin->setDecimals(2);
    }
    rate->setRange(0.001, 1000.0);
    rate->setDecimals(3);
    rulerRow->addWidget(new QLabel(QStringLiteral("Range"), this));
    rulerRow->addWidget(start);
    rulerRow->addWidget(new QLabel(QStringLiteral("to"), this));
    rulerRow->addWidget(end);
    rulerRow->addWidget(scrubber, 1);
    rulerRow->addWidget(new QLabel(QStringLiteral("FPS"), this));
    rulerRow->addWidget(rate);

    auto syncRange = [this, start, end, rate, scrubber] {
        QSignalBlocker a(start), b(end), c(rate), d(scrubber);
        start->setValue(m_transport->startValue());
        end->setValue(m_transport->endValue());
        rate->setValue(m_transport->rate());
        scrubber->setRange(qFloor(m_transport->startValue()), qCeil(m_transport->endValue()));
    };
    auto syncTime = [this, frameReadout, timeReadout, scrubber] {
        QSignalBlocker block(scrubber);
        scrubber->setValue(qRound(m_context->timeValue()));
        frameReadout->setText(QStringLiteral("Frame %1").arg(m_context->timeValue(), 0, 'f', 2));
        timeReadout->setText(QStringLiteral("%1 s").arg(m_context->timeSeconds(), 0, 'f', 3));
    };
    syncRange();
    syncTime();

    connect(first, &QPushButton::clicked, this, [this] {
        m_transport->seek(core::TimeValue(m_transport->startValue(), m_transport->rate()));
    });
    connect(previous, &QPushButton::clicked, this, [this] { m_transport->step(-1.0); });
    connect(next, &QPushButton::clicked, this, [this] { m_transport->step(1.0); });
    connect(last, &QPushButton::clicked, this, [this] {
        m_transport->seek(core::TimeValue(m_transport->endValue(), m_transport->rate()));
    });
    connect(play, &QPushButton::clicked, this,
        [this] { m_transport->setPlaying(!m_transport->playing()); });
    connect(scrubber, &QSlider::valueChanged, this,
        [this](int value) { m_transport->seek(core::TimeValue(value, m_transport->rate())); });
    connect(rate, &QDoubleSpinBox::valueChanged, m_transport, &core::TimeTransport::setRate);
    auto setRange = [this, start, end] {
        m_transport->setPlaybackRange(core::TimeValue(start->value(), m_transport->rate()),
            core::TimeValue(end->value(), m_transport->rate()));
    };
    connect(start, &QDoubleSpinBox::valueChanged, this, setRange);
    connect(end, &QDoubleSpinBox::valueChanged, this, setRange);
    connect(m_context, &core::TimeContext::changed, this, syncTime);
    connect(m_transport, &core::TimeTransport::playbackChanged, this, syncRange);
    connect(m_transport, &core::TimeTransport::playingChanged, this, [this, play](bool playing) {
        play->setText(playing ? QStringLiteral("Pause") : QStringLiteral("Play"));
        if (playing) {
            m_clock->restart();
            m_timer->start(qMax(1, qRound(1000.0 / m_transport->rate())));
        } else {
            m_timer->stop();
        }
    });
    connect(m_timer, &QTimer::timeout, this,
        [this] { m_transport->advanceSeconds(double(m_clock->restart()) / 1000.0); });
    if (m_transport->playing()) {
        play->setText(QStringLiteral("Pause"));
        m_clock->restart();
        m_timer->start(qMax(1, qRound(1000.0 / m_transport->rate())));
    }
}

} // namespace miskeyed::workbench::ui
