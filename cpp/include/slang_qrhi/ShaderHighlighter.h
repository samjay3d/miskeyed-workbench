#pragma once

#include "Export.h"
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <vector>

namespace slang_qrhi {

// Lightweight highlighter for Slang / HLSL / GLSL source, also used for the generated
// code viewer. Colors keywords, types, numbers, strings, comments, preprocessor lines,
// [attributes], : SEMANTICS and function calls with a Tokyo-Night style palette.
class SLANG_QRHI_EXPORT ShaderHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit ShaderHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule { QRegularExpression pattern; QTextCharFormat format; };
    std::vector<Rule> m_rules;
    QRegularExpression m_commentStart;
    QRegularExpression m_commentEnd;
    QTextCharFormat m_commentFormat;
};

} // namespace slang_qrhi
