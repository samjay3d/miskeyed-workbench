#pragma once

#include "Export.h"
#include "LspClient.h"
#include <QPlainTextEdit>
#include <QHash>
#include <QList>
#include <QPoint>

class QPaintEvent;
class QResizeEvent;
class QKeyEvent;
class QMouseEvent;
class QCompleter;
class QStringListModel;
class QTimer;

namespace slang_qrhi {

// A QPlainTextEdit with a gutter of line numbers and a highlighted current line.
// Used for both the editable Slang source and the read-only generated-code viewer.
// When given a LspClient it also provides IDE features driven by Slang's language
// server: auto-completion, hover docs, signature help, live diagnostic squiggles and
// Ctrl+Click go-to-definition.
class SLANG_QRHI_EXPORT CodeEditor final : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;

    // Connects this editor to the language server for the document identified by `uri`.
    void setLanguageClient(LspClient* client, const QString& uri);
    // Applies diagnostics (squiggle underlines + hover messages) from the server.
    void setDiagnostics(const QList<LspDiagnostic>& diagnostics);
    // Moves the caret to a 0-based line/character, centres it and focuses the editor.
    void goToPosition(int line, int character);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightCurrentLine();
    void insertCompletion(const QString& completion);
    void requestHoverAtCursor();

private:
    void rebuildExtraSelections();
    void triggerCompletion();
    void showCompletions(const QJsonObject& response);
    void showSignatureHelp();
    QString completionPrefix() const;
    QTextCursor cursorForRange(const LspRange& range) const;
    static bool rangeContains(const LspRange& range, int line, int character);

    QWidget* m_lineNumberArea = nullptr;

    LspClient* m_lsp = nullptr;
    QString m_uri;
    QCompleter* m_completer = nullptr;
    QStringListModel* m_completionModel = nullptr;
    QHash<QString, QString> m_completionInsert;   // label -> insertText
    QList<LspDiagnostic> m_diagnostics;
    QTimer* m_changeTimer = nullptr;              // debounces didChange to the server
    QTimer* m_autoCompleteTimer = nullptr;        // opens the popup after the user pauses typing
    QTimer* m_hoverTimer = nullptr;
    QPoint m_hoverPos;
    int m_completionSerial = 0;                   // drops stale async completion replies
};

} // namespace slang_qrhi
