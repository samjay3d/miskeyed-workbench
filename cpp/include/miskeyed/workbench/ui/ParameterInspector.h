#pragma once

#include <miskeyed/workbench/Export.h>
#include <miskeyed/workbench/slang/ShaderParameterModel.h>
#include <QWidget>

class QVBoxLayout;

namespace miskeyed::workbench::slang_rhi {

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ParameterInspector final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(ShaderParameterModel* model READ model WRITE setModel NOTIFY modelChanged)
public:
    explicit ParameterInspector(QWidget* parent = nullptr);
    ShaderParameterModel* model() const { return m_model; }
    void setModel(ShaderParameterModel* model);
    void setGroupFilter(const QString& group, bool include);

signals:
    void modelChanged();

private slots:
    void rebuild();

private:
    ShaderParameterModel* m_model = nullptr;
    QVBoxLayout* m_layout = nullptr;
    QString m_groupFilter;
    bool m_includeGroup = true;
};

} // namespace miskeyed::workbench::slang_rhi
