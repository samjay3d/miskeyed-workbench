#pragma once

#include "Export.h"
#include <QByteArray>
#include <QByteArrayView>
#include <QHashFunctions>
#include <QList>
#include <QPair>
#include <QString>

namespace slang_qrhi {

class SLANG_QRHI_EXPORT Digest final {
public:
    static constexpr qsizetype Size = 32;

    Digest();
    explicit Digest(QByteArray bytes);

    static Digest hash(QByteArrayView bytes);
    static Digest combine(QByteArrayView domain, QByteArrayView localPayload,
        const QList<QPair<QByteArray, Digest>>& dependencies);

    [[nodiscard]] QByteArray bytes() const { return m_bytes; }
    [[nodiscard]] QString hex() const { return QString::fromLatin1(m_bytes.toHex()); }
    [[nodiscard]] bool isNull() const;

    friend bool operator==(const Digest&, const Digest&) = default;

private:
    QByteArray m_bytes;
};

} // namespace slang_qrhi

inline size_t qHash(const slang_qrhi::Digest& d, size_t seed = 0)
{
    const QByteArray b = d.bytes();
    return qHashBits(b.constData(), static_cast<size_t>(b.size()), seed);
}
