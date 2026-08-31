#include <miskeyed/workbench/ui/ToolViewSelector.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>

namespace miskeyed::workbench::ui {

ToolViewSelector::ToolViewSelector(QWidget* parent)
    : QWidget(parent)
    , m_combo(new QComboBox(this))
{
    setObjectName(QStringLiteral("ToolSelector"));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(6, 2, 6, 2);
    row->setSpacing(4);
    row->addWidget(new QLabel(QStringLiteral("View:"), this));
    m_combo->setMinimumContentsLength(12);
    m_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    row->addWidget(m_combo);
    connect(m_combo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            emit viewSelected(m_combo->itemData(index).toString());
    });
}

void ToolViewSelector::clear()
{
    QSignalBlocker block(m_combo);
    m_combo->clear();
}

void ToolViewSelector::addView(const QString& id, const QString& title)
{
    QSignalBlocker block(m_combo);
    m_combo->addItem(title, id);
}

void ToolViewSelector::setCurrentView(const QString& id)
{
    const int index = m_combo->findData(id);
    if (index < 0 || index == m_combo->currentIndex())
        return;
    QSignalBlocker block(m_combo);
    m_combo->setCurrentIndex(index);
}

int ToolViewSelector::viewCount() const
{
    return m_combo->count();
}

} // namespace miskeyed::workbench::ui
