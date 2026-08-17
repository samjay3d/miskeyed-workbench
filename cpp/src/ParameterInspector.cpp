#include <slang_qrhi/ParameterInspector.h>
#include <slang_qrhi/ShaderParameterModel.h>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <memory>

namespace slang_qrhi {
namespace {

QWidget* makeEditor(ShaderParameterModel* model, int row, QWidget* parent)
{
    const QModelIndex idx = model->index(row);
    const auto type = ParameterType(model->data(idx, ShaderParameterModel::TypeRole).toInt());
    const QVariant value = model->data(idx, ShaderParameterModel::ValueRole);

    if (type == ParameterType::Bool) {
        auto* w = new QCheckBox(parent);
        w->setChecked(value.toBool());
        QObject::connect(w, &QCheckBox::toggled, model, [model, idx](bool v){ model->setData(idx, v, ShaderParameterModel::ValueRole); });
        return w;
    }
    if (type == ParameterType::Int || type == ParameterType::UInt) {
        auto* w = new QSpinBox(parent);
        const auto lo = model->data(idx, ShaderParameterModel::MinimumRole);
        const auto hi = model->data(idx, ShaderParameterModel::MaximumRole);
        w->setRange(lo.isValid()?lo.toInt():-100000, hi.isValid()?hi.toInt():100000);
        w->setValue(value.toInt());
        QObject::connect(w, &QSpinBox::valueChanged, model, [model, idx](int v){ model->setData(idx, v, ShaderParameterModel::ValueRole); });
        return w;
    }
    if (type == ParameterType::Float) {
        const auto lo = model->data(idx, ShaderParameterModel::MinimumRole);
        const auto hi = model->data(idx, ShaderParameterModel::MaximumRole);
        const double dmin = lo.isValid()?lo.toDouble():-1000.0;
        const double dmax = hi.isValid()?hi.toDouble():1000.0;
        const double step = model->data(idx, ShaderParameterModel::StepRole).toDouble();
        const QString widget = model->data(idx, ShaderParameterModel::WidgetRole).toString();

        auto* spin = new QDoubleSpinBox;
        spin->setRange(dmin, dmax);
        spin->setSingleStep(step > 0 ? step : 0.01);
        spin->setDecimals(6);
        spin->setValue(value.toDouble());

        // "slider"/"angle" widgets pair a bounded slider with the spin box; needs a range.
        const bool bounded = lo.isValid() && hi.isValid() && dmax > dmin;
        if (bounded && (widget == QLatin1String("slider") || widget == QLatin1String("angle"))) {
            auto* container = new QWidget(parent);
            auto* line = new QHBoxLayout(container); line->setContentsMargins(0,0,0,0);
            auto* slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(0, 1000);
            auto toTick = [dmin,dmax](double v){ return int(qBound(0.0, (v-dmin)/(dmax-dmin)*1000.0, 1000.0)); };
            auto toVal  = [dmin,dmax](int t){ return dmin + (double(t)/1000.0)*(dmax-dmin); };
            slider->setValue(toTick(value.toDouble()));
            spin->setParent(container);
            line->addWidget(slider, 1);
            line->addWidget(spin);
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, model, [model, idx, slider, toTick](double v){
                { QSignalBlocker b(slider); slider->setValue(toTick(v)); }
                model->setData(idx, v, ShaderParameterModel::ValueRole);
            });
            QObject::connect(slider, &QSlider::valueChanged, spin, [spin, toVal](int t){ spin->setValue(toVal(t)); });
            return container;
        }

        spin->setParent(parent);
        QObject::connect(spin, &QDoubleSpinBox::valueChanged, model, [model, idx](double v){ model->setData(idx, v, ShaderParameterModel::ValueRole); });
        return spin;
    }

    // Vector controls stay compact: one spin box per component.
    int count = type==ParameterType::Float2?2:type==ParameterType::Float3?3:type==ParameterType::Float4?4:0;
    if (count) {
        auto* container = new QWidget(parent);
        auto* line = new QHBoxLayout(container); line->setContentsMargins(0,0,0,0);
        QVector<double> vals(count);
        if (type==ParameterType::Float2) { auto v=value.value<QVector2D>(); vals={v.x(),v.y()}; }
        if (type==ParameterType::Float3) { auto v=value.value<QVector3D>(); vals={v.x(),v.y(),v.z()}; }
        if (type==ParameterType::Float4) { auto v=value.value<QVector4D>(); vals={v.x(),v.y(),v.z(),v.w()}; }
        auto shared = std::make_shared<QVector<double>>(vals);
        for (int c=0;c<count;++c) {
            auto* spin = new QDoubleSpinBox(container); spin->setRange(-1000,1000); spin->setDecimals(5); spin->setValue(vals[c]);
            line->addWidget(spin);
            QObject::connect(spin, &QDoubleSpinBox::valueChanged, model, [model, idx, shared, c, type](double x){
                (*shared)[c]=x;
                QVariant v;
                if (type==ParameterType::Float2) v=QVariant::fromValue(QVector2D(float((*shared)[0]),float((*shared)[1])));
                if (type==ParameterType::Float3) v=QVariant::fromValue(QVector3D(float((*shared)[0]),float((*shared)[1]),float((*shared)[2])));
                if (type==ParameterType::Float4) v=QVariant::fromValue(QVector4D(float((*shared)[0]),float((*shared)[1]),float((*shared)[2]),float((*shared)[3])));
                model->setData(idx,v,ShaderParameterModel::ValueRole);
            });
        }
        return container;
    }

    auto* unsupported = new QLabel(QStringLiteral("unsupported"), parent);
    unsupported->setEnabled(false);
    return unsupported;
}

} // namespace

ParameterInspector::ParameterInspector(QWidget* parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(6,6,6,6);
    m_layout->setAlignment(Qt::AlignTop);
}

void ParameterInspector::setModel(ShaderParameterModel* model)
{
    if (m_model == model) return;
    if (m_model) disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    if (m_model) {
        connect(m_model, &QAbstractItemModel::modelReset, this, &ParameterInspector::rebuild);
        connect(m_model, &ShaderParameterModel::layoutChangedByShader, this, &ParameterInspector::rebuild);
    }
    rebuild();
    emit modelChanged();
}

void ParameterInspector::rebuild()
{
    while (auto* item = m_layout->takeAt(0)) { if (item->widget()) item->widget()->deleteLater(); delete item; }
    if (!m_model) return;

    QMap<QString, QList<int>> groups;
    for (int row=0; row<m_model->rowCount(); ++row) groups[m_model->data(m_model->index(row), ShaderParameterModel::GroupRole).toString()].append(row);
    for (auto it=groups.cbegin(); it!=groups.cend(); ++it) {
        auto* box = new QGroupBox(it.key().isEmpty()?QStringLiteral("Parameters"):it.key(), this);
        auto* form = new QFormLayout(box);
        for (int row : it.value()) {
            const auto idx=m_model->index(row);
            const QString label=m_model->data(idx, ShaderParameterModel::LabelRole).toString();
            auto* editor=makeEditor(m_model,row,box);
            editor->setToolTip(m_model->data(idx, ShaderParameterModel::TooltipRole).toString());
            form->addRow(label, editor);
        }
        m_layout->addWidget(box);
    }
    m_layout->addStretch(1);
}

} // namespace slang_qrhi
