#include <slang_qrhi/ShaderParameterModel.h>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <cstring>

namespace slang_qrhi {

ShaderParameterModel::ShaderParameterModel(QObject* parent) : QAbstractListModel(parent) {}
int ShaderParameterModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : m_descriptors.size(); }

QHash<int, QByteArray> ShaderParameterModel::roleNames() const
{
    return {{NameRole,"name"},{LabelRole,"label"},{GroupRole,"group"},{WidgetRole,"widget"},{TooltipRole,"tooltip"},
            {TypeRole,"type"},{ValueRole,"value"},{MinimumRole,"minimum"},{MaximumRole,"maximum"},{StepRole,"step"},
            {ChoicesRole,"choices"},{OffsetRole,"offset"},{SizeRole,"size"}};
}

QVariant ShaderParameterModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_descriptors.size()) return {};
    const auto& d = m_descriptors[index.row()];
    switch (role) {
    case Qt::DisplayRole: case LabelRole: return d.label.isEmpty() ? d.name : d.label;
    case NameRole: return d.name; case GroupRole: return d.group; case WidgetRole: return d.widget;
    case TooltipRole: return d.tooltip; case TypeRole: return int(d.type); case ValueRole: return m_values[index.row()];
    case MinimumRole: return d.minimum; case MaximumRole: return d.maximum; case StepRole: return d.step;
    case ChoicesRole: return d.choices; case OffsetRole: return qlonglong(d.offset); case SizeRole: return qlonglong(d.size);
    default: return {};
    }
}

bool ShaderParameterModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role != ValueRole || !index.isValid()) return false;
    return writeValue(index.row(), value, true);
}

Qt::ItemFlags ShaderParameterModel::flags(const QModelIndex& index) const
{
    return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable : Qt::NoItemFlags;
}

void ShaderParameterModel::setDescriptors(QList<ParameterDescriptor> descriptors, qsizetype byteSize)
{
    QHash<QString, QVariant> preserved;
    QHash<QString, ParameterType> oldTypes;
    for (int i = 0; i < m_descriptors.size(); ++i) { preserved.insert(m_descriptors[i].name, m_values[i]); oldTypes.insert(m_descriptors[i].name, m_descriptors[i].type); }

    beginResetModel();
    m_descriptors = std::move(descriptors);
    m_values.clear();
    m_bytes = QByteArray(byteSize, '\0');
    for (int i = 0; i < m_descriptors.size(); ++i) {
        const auto& d = m_descriptors[i];
        QVariant v = d.defaultValue;
        if (preserved.contains(d.name) && oldTypes.value(d.name) == d.type) v = preserved.value(d.name);
        m_values.push_back(v);
        packValue(d, v, m_bytes);
    }
    endResetModel();
    emit layoutChangedByShader();
}

QVariant ShaderParameterModel::value(const QString& name) const
{
    for (int i = 0; i < m_descriptors.size(); ++i) if (m_descriptors[i].name == name) return m_values[i];
    return {};
}

bool ShaderParameterModel::setValue(const QString& name, const QVariant& value)
{
    for (int i = 0; i < m_descriptors.size(); ++i) if (m_descriptors[i].name == name) return writeValue(i, value, true);
    return false;
}

void ShaderParameterModel::resetValues()
{
    for (int i = 0; i < m_descriptors.size(); ++i) writeValue(i, m_descriptors[i].defaultValue, false);
    if (!m_bytes.isEmpty()) emit packedRangeChanged(0, int(m_bytes.size()));
    if (!m_descriptors.isEmpty()) emit dataChanged(index(0), index(m_descriptors.size()-1), {ValueRole});
}

bool ShaderParameterModel::writeValue(int row, const QVariant& value, bool emitSignals)
{
    if (row < 0 || row >= m_descriptors.size()) return false;
    const auto& d = m_descriptors[row];
    const QVariant normalized = normalizedValue(d, value);
    if (normalized == m_values[row]) return false;
    m_values[row] = normalized;
    if (!packValue(d, normalized, m_bytes)) return false;
    if (emitSignals) {
        emit dataChanged(index(row), index(row), {ValueRole});
        emit parameterChanged(d.name, normalized, int(d.offset), int(d.size));
        emit packedRangeChanged(int(d.offset), int(d.size));
    }
    return true;
}

QVariant ShaderParameterModel::normalizedValue(const ParameterDescriptor& d, const QVariant& value)
{
    if (d.type == ParameterType::Bool) return value.toBool();
    if (d.type == ParameterType::Float || d.type == ParameterType::Int || d.type == ParameterType::UInt) return value;
    return value;
}

bool ShaderParameterModel::packValue(const ParameterDescriptor& d, const QVariant& v, QByteArray& bytes)
{
    if (d.offset < 0 || d.offset + d.size > bytes.size()) return false;
    char* dst = bytes.data() + d.offset;
    auto f = [&](float x, int slot) { if (qsizetype((slot+1)*4) <= d.size) std::memcpy(dst + slot*4, &x, 4); };
    auto i = [&](qint32 x, int slot) { if (qsizetype((slot+1)*4) <= d.size) std::memcpy(dst + slot*4, &x, 4); };
    auto u = [&](quint32 x, int slot) { if (qsizetype((slot+1)*4) <= d.size) std::memcpy(dst + slot*4, &x, 4); };

    switch (d.type) {
    case ParameterType::Float: f(v.toFloat(),0); return true;
    case ParameterType::Int: i(v.toInt(),0); return true;
    case ParameterType::UInt: u(v.toUInt(),0); return true;
    case ParameterType::Bool: u(v.toBool()?1u:0u,0); return true;
    case ParameterType::Float2: { auto x=v.value<QVector2D>(); f(x.x(),0); f(x.y(),1); return true; }
    case ParameterType::Float3: { auto x=v.value<QVector3D>(); f(x.x(),0); f(x.y(),1); f(x.z(),2); return true; }
    case ParameterType::Float4: { auto x=v.value<QVector4D>(); f(x.x(),0); f(x.y(),1); f(x.z(),2); f(x.w(),3); return true; }
    default: return false;
    }
}

} // namespace slang_qrhi
