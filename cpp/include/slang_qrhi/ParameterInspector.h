#pragma once

#include "Export.h"
#include "ShaderParameterModel.h"
#include <QWidget>

class QVBoxLayout;

namespace slang_qrhi {

class SLANG_QRHI_EXPORT ParameterInspector final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(ShaderParameterModel* model READ model WRITE setModel NOTIFY modelChanged)
public:
    explicit ParameterInspector(QWidget* parent = nullptr);
    ShaderParameterModel* model() const { return m_model; }
    void setModel(ShaderParameterModel* model);

signals:
    void modelChanged();

private slots:
    void rebuild();

private:
    ShaderParameterModel* m_model = nullptr;
    QVBoxLayout* m_layout = nullptr;
};

} // namespace slang_qrhi
