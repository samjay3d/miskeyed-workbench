#pragma once

#include "Export.h"
#include <QAbstractListModel>
#include <QByteArray>
#include <QVariant>

namespace slang_qrhi {

enum class ParameterType : quint8 {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Bool,
    Unknown,
};

struct ParameterDescriptor {
    QString name;
    QString label;
    QString group;
    QString widget;
    QString tooltip;
    ParameterType type = ParameterType::Unknown;
    qsizetype offset = 0;
    qsizetype size = 0;
    QVariant minimum;
    QVariant maximum;
    QVariant step;
    QVariant defaultValue;
    QStringList choices;
};

class SLANG_QRHI_EXPORT ShaderParameterModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int byteSize READ byteSize NOTIFY layoutChangedByShader)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        GroupRole,
        WidgetRole,
        TooltipRole,
        TypeRole,
        ValueRole,
        MinimumRole,
        MaximumRole,
        StepRole,
        ChoicesRole,
        OffsetRole,
        SizeRole,
    };
    Q_ENUM(Role)

    explicit ShaderParameterModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDescriptors(QList<ParameterDescriptor> descriptors, qsizetype byteSize);
    [[nodiscard]] QByteArray packedBytes() const { return m_bytes; }
    [[nodiscard]] int byteSize() const { return int(m_bytes.size()); }

    Q_INVOKABLE QVariant value(const QString& name) const;
    Q_INVOKABLE bool setValue(const QString& name, const QVariant& value);
    Q_INVOKABLE void resetValues();

signals:
    void parameterChanged(QString name, QVariant value, int offset, int size);
    void packedRangeChanged(int offset, int size);
    void layoutChangedByShader();

private:
    bool writeValue(int row, const QVariant& value, bool emitSignals);
    static QVariant normalizedValue(const ParameterDescriptor& d, const QVariant& value);
    static bool packValue(const ParameterDescriptor& d, const QVariant& value, QByteArray& bytes);

    QList<ParameterDescriptor> m_descriptors;
    QList<QVariant> m_values;
    QByteArray m_bytes;
};

} // namespace slang_qrhi
