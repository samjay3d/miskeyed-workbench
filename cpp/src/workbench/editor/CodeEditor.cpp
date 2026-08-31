#include <miskeyed/workbench/editor/CodeEditor.h>
#include <QAbstractItemView>
#include <QCompleter>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>
#include <QToolTip>

namespace miskeyed::workbench::slang_rhi {

namespace {
    // Gutter widget: forwards its paint/width to the owning editor.
    class LineNumberArea final : public QWidget {
    public:
        explicit LineNumberArea(CodeEditor* editor)
            : QWidget(editor)
            , m_editor(editor)
        {
        }
        QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }

    protected:
        void paintEvent(QPaintEvent* event) override { m_editor->lineNumberAreaPaintEvent(event); }

    private:
        CodeEditor* m_editor;
    };
} // namespace

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameShape(QFrame::NoFrame);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    rebuildExtraSelections();
}

void CodeEditor::rebuildExtraSelections()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(0x2a, 0x2c, 0x36));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    for (const LspDiagnostic& d : m_diagnostics) {
        QTextCursor cursor = cursorForRange(d.range);
        if (cursor.isNull() || !cursor.hasSelection())
            continue;
        const QColor color = d.severity == 1 ? QColor(0xf7, 0x76, 0x8e)
            : d.severity == 2                ? QColor(0xe0, 0xaf, 0x68)
                                             : QColor(0x7a, 0xa2, 0xf7);
        QTextEdit::ExtraSelection selection;
        selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        selection.format.setUnderlineColor(color);
        selection.cursor = cursor;
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x1b, 0x1c, 0x22));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int curLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool current = (blockNumber == curLine);
            painter.setPen(current ? QColor(0xc0, 0xca, 0xf5) : QColor(0x54, 0x5a, 0x75));
            painter.drawText(0, top, m_lineNumberArea->width() - 8, fontMetrics().height(),
                Qt::AlignRight, QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---------------------------------------------------------------------------
// Language-server driven features
// ---------------------------------------------------------------------------

void CodeEditor::setLanguageClient(LspClient* client, const QString& uri)
{
    m_lsp = client;
    m_uri = uri;
    if (!m_lsp)
        return;

    if (!m_completer) {
        m_completionModel = new QStringListModel(this);
        m_completer = new QCompleter(this);
        m_completer->setModel(m_completionModel);
        m_completer->setWidget(this);
        m_completer->setCompletionMode(QCompleter::PopupCompletion);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
        connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated), this,
            &CodeEditor::insertCompletion);
    }
    if (!m_changeTimer) {
        m_changeTimer = new QTimer(this);
        m_changeTimer->setSingleShot(true);
        m_changeTimer->setInterval(250);
        connect(m_changeTimer, &QTimer::timeout, this, [this] {
            if (m_lsp && !m_uri.isEmpty())
                m_lsp->updateDocument(m_uri, toPlainText());
        });
        connect(this, &QPlainTextEdit::textChanged, this, [this] {
            if (m_changeTimer)
                m_changeTimer->start();
        });
    }
    if (!m_autoCompleteTimer) {
        m_autoCompleteTimer = new QTimer(this);
        m_autoCompleteTimer->setSingleShot(true);
        m_autoCompleteTimer->setInterval(300);
        connect(m_autoCompleteTimer, &QTimer::timeout, this, [this] {
            if (!m_lsp || m_uri.isEmpty())
                return;
            if (m_completer && m_completer->popup()->isVisible())
                return; // already open; live-filter handles it
            if (completionPrefix().isEmpty())
                return; // nothing to match yet
            if (m_changeTimer)
                m_changeTimer->stop();
            m_lsp->updateDocument(
                m_uri, toPlainText()); // make sure the server sees the latest text first
            triggerCompletion();
        });
    }
    if (!m_hoverTimer) {
        setMouseTracking(true);
        m_hoverTimer = new QTimer(this);
        m_hoverTimer->setSingleShot(true);
        m_hoverTimer->setInterval(450);
        connect(m_hoverTimer, &QTimer::timeout, this, &CodeEditor::requestHoverAtCursor);
    }
}

void CodeEditor::setDiagnostics(const QList<LspDiagnostic>& diagnostics)
{
    m_diagnostics = diagnostics;
    rebuildExtraSelections();
}

void CodeEditor::goToPosition(int line, int character)
{
    const QTextBlock block = document()->findBlockByNumber(line);
    if (!block.isValid())
        return;
    QTextCursor cursor(document());
    cursor.setPosition(block.position() + qBound(0, character, block.length() - 1));
    setTextCursor(cursor);
    centerCursor();
    setFocus();
}

QTextCursor CodeEditor::cursorForRange(const LspRange& range) const
{
    const QTextBlock startBlock = document()->findBlockByNumber(range.startLine);
    if (!startBlock.isValid())
        return QTextCursor();
    const QTextBlock endBlock = document()->findBlockByNumber(range.endLine);
    const int start = startBlock.position() + qBound(0, range.startChar, startBlock.length() - 1);
    int end = start;
    if (endBlock.isValid())
        end = endBlock.position() + qBound(0, range.endChar, endBlock.length() - 1);
    if (end <= start)
        end = start + 1; // keep at least one character underlined
    QTextCursor cursor(document());
    cursor.setPosition(start);
    cursor.setPosition(qMin(end, document()->characterCount() - 1), QTextCursor::KeepAnchor);
    return cursor;
}

bool CodeEditor::rangeContains(const LspRange& range, int line, int character)
{
    if (line < range.startLine || line > range.endLine)
        return false;
    if (line == range.startLine && character < range.startChar)
        return false;
    if (line == range.endLine && character > range.endChar)
        return false;
    return true;
}

QString CodeEditor::completionPrefix() const
{
    const QString text = textCursor().block().text();
    const int col = textCursor().positionInBlock();
    int start = col;
    while (start > 0 && (text[start - 1].isLetterOrNumber() || text[start - 1] == QLatin1Char('_')))
        --start;
    return text.mid(start, col - start);
}

void CodeEditor::triggerCompletion()
{
    if (!m_lsp)
        return;
    const QTextCursor cursor = textCursor();
    const int serial = ++m_completionSerial;
    m_lsp->requestCompletion(m_uri, cursor.blockNumber(), cursor.positionInBlock(),
        [this, serial](const QJsonObject& response) {
            if (serial != m_completionSerial)
                return; // a newer request superseded this one
            showCompletions(response);
        });
}

void CodeEditor::showCompletions(const QJsonObject& response)
{
    QJsonValue result = response.value(QStringLiteral("result"));
    QJsonArray items;
    if (result.isArray())
        items = result.toArray();
    else if (result.isObject())
        items = result.toObject().value(QStringLiteral("items")).toArray();
    if (items.isEmpty()) {
        if (m_completer)
            m_completer->popup()->hide();
        return;
    }

    QStringList labels;
    labels.reserve(items.size());
    m_completionInsert.clear();
    for (const QJsonValue& v : items) {
        const QJsonObject item = v.toObject();
        const QString label = item.value(QStringLiteral("label")).toString().trimmed();
        if (label.isEmpty())
            continue;
        QString insert = item.value(QStringLiteral("insertText")).toString();
        if (insert.isEmpty())
            insert = label;
        labels.append(label);
        m_completionInsert.insert(label, insert);
    }
    labels.removeDuplicates();
    labels.sort(Qt::CaseInsensitive);
    m_completionModel->setStringList(labels);

    m_completer->setCompletionPrefix(completionPrefix());
    if (m_completer->completionCount() == 0) {
        m_completer->popup()->hide();
        return;
    }

    QRect rect = cursorRect();
    rect.setWidth(m_completer->popup()->sizeHintForColumn(0)
        + m_completer->popup()->verticalScrollBar()->sizeHint().width() + 24);
    m_completer->complete(rect);
}

void CodeEditor::insertCompletion(const QString& completion)
{
    const QString insert = m_completionInsert.value(completion, completion);
    QTextCursor cursor = textCursor();
    const int prefixLen = completionPrefix().length();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefixLen);
    cursor.insertText(insert);
    setTextCursor(cursor);
}

void CodeEditor::showSignatureHelp()
{
    if (!m_lsp)
        return;
    const QTextCursor cursor = textCursor();
    m_lsp->requestSignatureHelp(
        m_uri, cursor.blockNumber(), cursor.positionInBlock(), [this](const QJsonObject& response) {
            const QJsonObject result = response.value(QStringLiteral("result")).toObject();
            const QJsonArray signatures = result.value(QStringLiteral("signatures")).toArray();
            if (signatures.isEmpty())
                return;
            const int active = qBound(
                0, result.value(QStringLiteral("activeSignature")).toInt(), signatures.size() - 1);
            const QString label
                = signatures[active].toObject().value(QStringLiteral("label")).toString();
            if (label.isEmpty())
                return;
            QToolTip::showText(mapToGlobal(cursorRect().bottomLeft()), label, this);
        });
}

void CodeEditor::requestHoverAtCursor()
{
    if (!m_lsp)
        return;
    const QTextCursor cursor = cursorForPosition(m_hoverPos);
    const int line = cursor.blockNumber();
    const int character = cursor.positionInBlock();

    // A diagnostic under the cursor takes precedence — show its message immediately.
    for (const LspDiagnostic& d : m_diagnostics) {
        if (rangeContains(d.range, line, character)) {
            QToolTip::showText(mapToGlobal(m_hoverPos), d.message, this);
            return;
        }
    }
    const QPoint at = m_hoverPos;
    m_lsp->requestHover(m_uri, line, character, [this, at](const QJsonObject& response) {
        const QJsonObject result = response.value(QStringLiteral("result")).toObject();
        const QJsonValue contents = result.value(QStringLiteral("contents"));
        QString text;
        if (contents.isString())
            text = contents.toString();
        else if (contents.isObject())
            text = contents.toObject().value(QStringLiteral("value")).toString();
        else if (contents.isArray()) {
            for (const QJsonValue& v : contents.toArray()) {
                if (v.isString())
                    text += v.toString();
                else if (v.isObject())
                    text += v.toObject().value(QStringLiteral("value")).toString();
                text += QLatin1Char('\n');
            }
        }
        text = text.trimmed();
        if (text.isEmpty())
            return;
        // slangd returns markdown (code fences, ---, backticks); render it as rich text
        // so signatures and doc strings read cleanly instead of showing raw markup.
        QTextDocument md;
        md.setMarkdown(text);
        const QString html
            = QStringLiteral("<div style='max-width:520px'>%1</div>").arg(md.toHtml());
        QToolTip::showText(mapToGlobal(at), html, this);
    });
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        case Qt::Key_Escape:
            event->ignore();
            return; // let the popup consume it
        default:
            break;
        }
    }

    const bool triggerCompletionShortcut
        = (event->key() == Qt::Key_Space) && (event->modifiers() & Qt::ControlModifier);
    if (triggerCompletionShortcut) {
        triggerCompletion();
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
    if (!m_lsp)
        return;

    const QString typed = event->text();
    if (typed.isEmpty())
        return;
    const QChar ch = typed.at(0);
    if (ch == QLatin1Char('.')) {
        triggerCompletion();
    } else if (ch == QLatin1Char('(') || ch == QLatin1Char(',')) {
        showSignatureHelp();
    } else if (m_completer && m_completer->popup()->isVisible()) {
        m_completer->setCompletionPrefix(completionPrefix());
        if (m_completer->completionCount() == 0)
            m_completer->popup()->hide();
        else
            m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    } else if ((ch.isLetterOrNumber() || ch == QLatin1Char('_')) && m_autoCompleteTimer) {
        m_autoCompleteTimer
            ->start(); // pause-to-complete: open the popup at the caret once typing stops
    }
}

void CodeEditor::mouseMoveEvent(QMouseEvent* event)
{
    QPlainTextEdit::mouseMoveEvent(event);
    if (!m_lsp)
        return;
    m_hoverPos = event->pos();
    QToolTip::hideText();
    if (m_hoverTimer)
        m_hoverTimer->start();
}

void CodeEditor::mousePressEvent(QMouseEvent* event)
{
    if (m_lsp && event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)) {
        const QTextCursor cursor = cursorForPosition(event->pos());
        m_lsp->requestDefinition(m_uri, cursor.blockNumber(), cursor.positionInBlock(),
            [this](const QJsonObject& response) {
                QJsonValue result = response.value(QStringLiteral("result"));
                QJsonObject location;
                if (result.isArray() && !result.toArray().isEmpty())
                    location = result.toArray().first().toObject();
                else if (result.isObject())
                    location = result.toObject();
                if (location.isEmpty())
                    return;
                if (location.value(QStringLiteral("uri")).toString() != m_uri)
                    return; // same-document jumps only
                const QJsonObject start = location.value(QStringLiteral("range"))
                                              .toObject()
                                              .value(QStringLiteral("start"))
                                              .toObject();
                const QTextBlock block
                    = document()->findBlockByNumber(start.value(QStringLiteral("line")).toInt());
                if (!block.isValid())
                    return;
                QTextCursor target(document());
                target.setPosition(block.position()
                    + qBound(
                        0, start.value(QStringLiteral("character")).toInt(), block.length() - 1));
                setTextCursor(target);
                centerCursor();
            });
        return;
    }
    QPlainTextEdit::mousePressEvent(event);
}

} // namespace miskeyed::workbench::slang_rhi
