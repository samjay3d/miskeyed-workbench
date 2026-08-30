#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>
#include <QList>
#include <QUrl>

namespace miskeyed::workbench::slang_rhi {

class ShaderDocument;

// Cheap editor/view state retained per open document. Widgets may be reused; switching
// focus saves/restores this session instead of treating one editor as the document.
struct DocumentSession final {
    ShaderDocument* document = nullptr;
    QString displayName;
    int cursorPosition = 0;
    int anchorPosition = 0;
    int verticalScroll = 0;
    int horizontalScroll = 0;
    int viewMode = 0; // Source / Generated / Compare
    QString generatedTarget;
};

// Owns open authoring documents, their view sessions, and exactly one focus. Runtime
// bindings belong to mode sessions (Render Toy, Lookdev, etc.), never to this workspace.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ShaderWorkspace final : public QObject {
    Q_OBJECT
public:
    explicit ShaderWorkspace(QObject* parent = nullptr);

    int documentCount() const { return m_sessions.size(); }
    ShaderDocument* documentAt(int index) const;
    QString displayName(ShaderDocument* document) const;
    ShaderDocument* focusedDocument() const { return m_focused; }
    DocumentSession* session(ShaderDocument* document);
    const DocumentSession* session(ShaderDocument* document) const;

    ShaderDocument* openSource(
        const QUrl& identity, const QString& displayName, const QString& source);
    ShaderDocument* openFile(const QString& path);
    void focusDocument(ShaderDocument* document);
    bool closeDocument(ShaderDocument* document);
    bool moveDocument(int from, int to);

signals:
    void documentAdded(ShaderDocument* document);
    void documentChanged(ShaderDocument* document);
    void documentAboutToClose(ShaderDocument* document);
    void documentClosed();
    void documentOrderChanged();
    void focusChanging(ShaderDocument* previous, ShaderDocument* next);
    void focusedDocumentChanged(ShaderDocument* document);

private:
    int indexOf(ShaderDocument* document) const;

    QList<DocumentSession> m_sessions;
    ShaderDocument* m_focused = nullptr;
};

} // namespace miskeyed::workbench::slang_rhi
