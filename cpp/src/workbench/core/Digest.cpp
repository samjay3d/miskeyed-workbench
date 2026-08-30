#include <miskeyed/workbench/core/Digest.h>
#include <QtEndian>
#include <array>
#include <cstring>
#include <stdexcept>

namespace miskeyed::workbench::slang_rhi {
namespace {

    // Compact BLAKE2b implementation for unkeyed 32-byte digests. This is a fresh
    // implementation of RFC 7693 semantics; it intentionally matches
    // hashlib.blake2b(data, digest_size=32).
    constexpr std::array<quint64, 8> IV = {
        0x6a09e667f3bcc908ULL,
        0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL,
        0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,
        0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL,
        0x5be0cd19137e2179ULL,
    };
    constexpr quint8 SIGMA[12][16] = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
        { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
        { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
        { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
        { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
        { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
        { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
        { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
        { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    };

    inline quint64 rotr(quint64 x, int n)
    {
        return (x >> n) | (x << (64 - n));
    }

    struct Blake2b {
        quint64 h[8] {};
        quint64 t0 = 0, t1 = 0;
        quint64 f0 = 0;
        std::array<quint8, 128> buffer {};
        size_t used = 0;

        Blake2b()
        {
            std::copy(IV.begin(), IV.end(), h);
            h[0] ^= 0x01010000ULL ^ 32ULL; // fanout=1, depth=1, output=32
        }

        void increment(quint64 n)
        {
            const quint64 old = t0;
            t0 += n;
            if (t0 < old)
                ++t1;
        }

        void compress(const quint8* block)
        {
            quint64 m[16], v[16];
            for (int i = 0; i < 16; ++i)
                m[i] = qFromLittleEndian<quint64>(block + i * 8);
            for (int i = 0; i < 8; ++i) {
                v[i] = h[i];
                v[i + 8] = IV[i];
            }
            v[12] ^= t0;
            v[13] ^= t1;
            v[14] ^= f0;
            auto G = [&](int r, int i, int a, int b, int c, int d) {
                v[a] += v[b] + m[SIGMA[r][2 * i]];
                v[d] = rotr(v[d] ^ v[a], 32);
                v[c] += v[d];
                v[b] = rotr(v[b] ^ v[c], 24);
                v[a] += v[b] + m[SIGMA[r][2 * i + 1]];
                v[d] = rotr(v[d] ^ v[a], 16);
                v[c] += v[d];
                v[b] = rotr(v[b] ^ v[c], 63);
            };
            for (int r = 0; r < 12; ++r) {
                G(r, 0, 0, 4, 8, 12);
                G(r, 1, 1, 5, 9, 13);
                G(r, 2, 2, 6, 10, 14);
                G(r, 3, 3, 7, 11, 15);
                G(r, 4, 0, 5, 10, 15);
                G(r, 5, 1, 6, 11, 12);
                G(r, 6, 2, 7, 8, 13);
                G(r, 7, 3, 4, 9, 14);
            }
            for (int i = 0; i < 8; ++i)
                h[i] ^= v[i] ^ v[i + 8];
        }

        void update(QByteArrayView input)
        {
            const auto* p = reinterpret_cast<const quint8*>(input.data());
            size_t n = size_t(input.size());
            while (n) {
                const size_t take = std::min(n, buffer.size() - used);
                std::memcpy(buffer.data() + used, p, take);
                used += take;
                p += take;
                n -= take;
                if (used == buffer.size() && n > 0) {
                    increment(128);
                    compress(buffer.data());
                    used = 0;
                }
            }
        }

        QByteArray finish()
        {
            increment(quint64(used));
            f0 = ~quint64(0);
            std::fill(buffer.begin() + qsizetype(used), buffer.end(), 0);
            compress(buffer.data());
            QByteArray out(32, Qt::Uninitialized);
            quint8 tmp[64];
            for (int i = 0; i < 8; ++i)
                qToLittleEndian<quint64>(h[i], tmp + i * 8);
            std::memcpy(out.data(), tmp, 32);
            return out;
        }
    };

    void appendFramed(Blake2b& b, QByteArrayView x)
    {
        quint64 n = qToLittleEndian<quint64>(quint64(x.size()));
        b.update(QByteArrayView(reinterpret_cast<const char*>(&n), sizeof(n)));
        b.update(x);
    }

} // namespace

Digest::Digest()
    : m_bytes(Size, '\0')
{
}
Digest::Digest(QByteArray bytes)
    : m_bytes(std::move(bytes))
{
    if (m_bytes.size() != Size)
        throw std::invalid_argument("Digest must be exactly 32 bytes");
}

Digest Digest::hash(QByteArrayView bytes)
{
    Blake2b b;
    b.update(bytes);
    return Digest(b.finish());
}

Digest Digest::combine(QByteArrayView domain, QByteArrayView localPayload,
    const QList<QPair<QByteArray, Digest>>& dependencies)
{
    Blake2b b;
    appendFramed(b, domain);
    appendFramed(b, localPayload);
    for (const auto& [label, digest] : dependencies) {
        appendFramed(b, label);
        appendFramed(b, digest.bytes());
    }
    return Digest(b.finish());
}

bool Digest::isNull() const
{
    for (char c : m_bytes)
        if (c != 0)
            return false;
    return true;
}

} // namespace miskeyed::workbench::slang_rhi
