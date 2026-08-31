#pragma once

#include <miskeyed/workbench/Export.h>
#include <QWidget>

class QComboBox;

namespace miskeyed::workbench::ui {

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ToolViewSelector final : public QWidget {
    Q_OBJECT
public:
    explicit ToolViewSelector(QWidget* parent = nullptr);
    void clear();
    void addView(const QString& id, const QString& title);
    void setCurrentView(const QString& id);
    int viewCount() const;

signals:
    void viewSelected(const QString& id);

private:
    QComboBox* m_combo = nullptr;
};

} // namespace miskeyed::workbench::ui
