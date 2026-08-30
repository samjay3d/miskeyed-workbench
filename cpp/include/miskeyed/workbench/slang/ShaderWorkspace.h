#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>
#include <QList>
#include <QUrl>

namespace miskeyed::workbench::slang_rhi {

class ShaderDocument;

enum class ShaderRole : quint8 { Generic, Scene, Post };

// Owns cheap authoring documents and the two bindings consumed by Render Toy. GPU
// state deliberately stays in SlangRhiWidget, so opening a tab does not allocate a
// pipeline or render target.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ShaderWorkspace final : public QObject {
    Q_OBJECT
public:
    explicit ShaderWorkspace(QObject* parent = nullptr);

    int documentCount() const { return m_documents.size(); }
    ShaderDocument* documentAt(int index) const;
    ShaderRole role(ShaderDocument* document) const;
    QString displayName(ShaderDocument* document) const;
    ShaderDocument* focusedDocument() const { return m_focused; }
    ShaderDocument* activeSceneDocument() const { return m_scene; }
    ShaderDocument* activePostDocument() const { return m_post; }

    ShaderDocument* openSource(
        const QUrl& identity, const QString& displayName, ShaderRole role, const QString& source);
    ShaderDocument* openFile(const QString& path, ShaderRole role = ShaderRole::Generic);
    void focusDocument(ShaderDocument* document);
    void bindScene(ShaderDocument* document);
    void bindPost(ShaderDocument* document);

signals:
    void documentAdded(ShaderDocument* document);
    void documentChanged(ShaderDocument* document);
    void focusedDocumentChanged(ShaderDocument* document);
    void renderBindingsChanged(ShaderDocument* scene, ShaderDocument* post);

private:
    struct Entry {
        ShaderDocument* document = nullptr;
        QString name;
        ShaderRole role = ShaderRole::Generic;
    };
    Entry* entry(ShaderDocument* document);
    const Entry* entry(ShaderDocument* document) const;

    QList<Entry> m_documents;
    ShaderDocument* m_focused = nullptr;
    ShaderDocument* m_scene = nullptr;
    ShaderDocument* m_post = nullptr;
};

} // namespace miskeyed::workbench::slang_rhi
