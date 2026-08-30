#include <miskeyed/workbench/editor/ShaderHighlighter.h>

namespace miskeyed::workbench::slang_rhi {

ShaderHighlighter::ShaderHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    const QColor kKeyword(0xbb, 0x9a, 0xf7); // purple
    const QColor kType(0x7d, 0xcf, 0xff); // cyan
    const QColor kNumber(0xff, 0x9e, 0x64); // orange
    const QColor kString(0x9e, 0xce, 0x6a); // green
    const QColor kComment(0x56, 0x5f, 0x89); // muted blue-grey
    const QColor kPreproc(0xf7, 0x76, 0x8e); // red
    const QColor kFunction(0x7a, 0xa2, 0xf7); // blue
    const QColor kAttr(0xe0, 0xaf, 0x68); // yellow

    auto add = [this](const QString& p, const QColor& c, bool italic = false, bool bold = false) {
        Rule r;
        r.pattern = QRegularExpression(p);
        r.format.setForeground(c);
        if (italic)
            r.format.setFontItalic(true);
        if (bold)
            r.format.setFontWeight(QFont::DemiBold);
        m_rules.push_back(std::move(r));
    };

    // Function calls: name immediately followed by '('.
    add(QStringLiteral("\\b[A-Za-z_]\\w*(?=\\s*\\()"), kFunction);

    // Types (HLSL/Slang/GLSL).
    static const char* const types[] = {
        "void",
        "bool",
        "int",
        "uint",
        "half",
        "float",
        "double",
        "float2",
        "float3",
        "float4",
        "float2x2",
        "float3x3",
        "float4x4",
        "float3x4",
        "float4x3",
        "int2",
        "int3",
        "int4",
        "uint2",
        "uint3",
        "uint4",
        "half2",
        "half3",
        "half4",
        "double2",
        "double3",
        "double4",
        "bool2",
        "bool3",
        "bool4",
        "matrix",
        "vector",
        "vec2",
        "vec3",
        "vec4",
        "ivec2",
        "ivec3",
        "ivec4",
        "mat2",
        "mat3",
        "mat4",
        "sampler2D",
        "Texture1D",
        "Texture2D",
        "Texture3D",
        "TextureCube",
        "Texture2DArray",
        "TextureCubeArray",
        "RWTexture2D",
        "RWTexture3D",
        "SamplerState",
        "SamplerComparisonState",
        "StructuredBuffer",
        "RWStructuredBuffer",
        "ConstantBuffer",
        "ByteAddressBuffer",
        "Buffer",
    };
    QString typeAlt;
    for (const char* t : types) {
        if (!typeAlt.isEmpty())
            typeAlt += QLatin1Char('|');
        typeAlt += QLatin1String(t);
    }
    add(QStringLiteral("\\b(?:%1)\\b").arg(typeAlt), kType, false, true);

    // Keywords.
    static const char* const keywords[] = {
        "if",
        "else",
        "for",
        "while",
        "do",
        "switch",
        "case",
        "default",
        "break",
        "continue",
        "return",
        "discard",
        "struct",
        "cbuffer",
        "tbuffer",
        "typedef",
        "namespace",
        "import",
        "module",
        "using",
        "in",
        "out",
        "inout",
        "ref",
        "const",
        "static",
        "uniform",
        "varying",
        "groupshared",
        "precise",
        "export",
        "public",
        "private",
        "internal",
        "extension",
        "interface",
        "associatedtype",
        "this",
        "true",
        "false",
        "nullptr",
        "let",
        "var",
        "func",
        "property",
        "enum",
        "typealias",
        "where",
        "is",
        "as",
        "new",
        "sizeof",
        "register",
        "packoffset",
        "layout",
        "flat",
        "noperspective",
        "centroid",
        "sample",
        "column_major",
        "row_major",
        "volatile",
        "globallycoherent",
        "snorm",
        "unorm",
    };
    QString kwAlt;
    for (const char* k : keywords) {
        if (!kwAlt.isEmpty())
            kwAlt += QLatin1Char('|');
        kwAlt += QLatin1String(k);
    }
    add(QStringLiteral("\\b(?:%1)\\b").arg(kwAlt), kKeyword, false, true);

    // Numbers (int/float/hex, with common suffixes).
    add(QStringLiteral("\\b0[xX][0-9a-fA-F]+\\b"), kNumber);
    add(QStringLiteral("\\b\\d+\\.?\\d*(?:[eE][+-]?\\d+)?[fFhHuUlL]*\\b"), kNumber);
    add(QStringLiteral("\\B\\.\\d+(?:[eE][+-]?\\d+)?[fFhH]?\\b"), kNumber);

    // : SEMANTIC (SV_Position, TEXCOORD0, SV_Target0, ...).
    add(QStringLiteral(":\\s*[A-Za-z_]\\w*"), kAttr);

    // [attributes] such as [shader(\"vertex\")], [[vk::binding(1)]].
    add(QStringLiteral("\\[\\[?[^\\]]*\\]?\\]"), kAttr, false, true);

    // Strings.
    add(QStringLiteral("\"[^\"\\n]*\""), kString);

    // Preprocessor lines.
    add(QStringLiteral("^\\s*#\\s*\\w+"), kPreproc, false, true);

    // Single-line comments (added last so they win over the above on overlap).
    add(QStringLiteral("//[^\\n]*"), kComment, true);

    m_commentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_commentEnd = QRegularExpression(QStringLiteral("\\*/"));
    m_commentFormat.setForeground(kComment);
    m_commentFormat.setFontItalic(true);
}

void ShaderHighlighter::highlightBlock(const QString& text)
{
    for (const Rule& r : m_rules) {
        auto it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }

    // Multi-line /* ... */ comments across blocks.
    setCurrentBlockState(0);
    int start = 0;
    if (previousBlockState() != 1)
        start = text.indexOf(m_commentStart);
    while (start >= 0) {
        const auto end = m_commentEnd.match(text, start);
        int length;
        if (!end.hasMatch()) {
            setCurrentBlockState(1);
            length = text.length() - start;
        } else {
            length = end.capturedEnd() - start;
        }
        setFormat(start, length, m_commentFormat);
        start = text.indexOf(m_commentStart, start + length);
    }
}

} // namespace miskeyed::workbench::slang_rhi
