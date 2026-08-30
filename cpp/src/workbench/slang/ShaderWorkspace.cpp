#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <QFileInfo>

namespace miskeyed::workbench::slang_rhi {

ShaderWorkspace::ShaderWorkspace(QObject* parent)
    : QObject(parent)
{
}

ShaderDocument* ShaderWorkspace::documentAt(int index) const
{
    return index >= 0 && index < m_documents.size() ? m_documents.at(index).document : nullptr;
}

ShaderWorkspace::Entry* ShaderWorkspace::entry(ShaderDocument* document)
{
    for (Entry& item : m_documents)
        if (item.document == document)
            return &item;
    return nullptr;
}

const ShaderWorkspace::Entry* ShaderWorkspace::entry(ShaderDocument* document) const
{
    for (const Entry& item : m_documents)
        if (item.document == document)
            return &item;
    return nullptr;
}

ShaderRole ShaderWorkspace::role(ShaderDocument* document) const
{
    const Entry* item = entry(document);
    return item ? item->role : ShaderRole::Generic;
}

QString ShaderWorkspace::displayName(ShaderDocument* document) const
{
    const Entry* item = entry(document);
    return item ? item->name : QString();
}

ShaderDocument* ShaderWorkspace::openSource(
    const QUrl& identity, const QString& displayName, ShaderRole role, const QString& source)
{
    for (const Entry& item : m_documents) {
        if (item.document->fileUrl() == identity) {
            focusDocument(item.document);
            if (item.role == ShaderRole::Scene)
                bindScene(item.document);
            else if (item.role == ShaderRole::Post)
                bindPost(item.document);
            return item.document;
        }
    }

    auto* document = new ShaderDocument(this);
    document->setFileUrl(identity);
    document->setSource(source);
    document->markSourceClean();
    m_documents.push_back({ document, displayName, role });
    connect(document, &ShaderDocument::sourceChanged, this,
        [this, document] { emit documentChanged(document); });
    emit documentAdded(document);
    focusDocument(document);
    if (role == ShaderRole::Scene)
        bindScene(document);
    else if (role == ShaderRole::Post)
        bindPost(document);
    return document;
}

ShaderDocument* ShaderWorkspace::openFile(const QString& path, ShaderRole role)
{
    const QFileInfo info(path);
    if (!info.exists())
        return nullptr;
    if (role == ShaderRole::Generic) {
        const QString lower = info.fileName().toLower();
        if (lower.startsWith(QStringLiteral("scene")))
            role = ShaderRole::Scene;
        else if (lower.startsWith(QStringLiteral("post")))
            role = ShaderRole::Post;
    }
    auto* document = openSource(
        QUrl::fromLocalFile(info.absoluteFilePath()), info.fileName(), role, QString());
    if (!document->load())
        return nullptr;
    document->compile();
    if (role == ShaderRole::Scene)
        bindScene(document);
    else if (role == ShaderRole::Post)
        bindPost(document);
    return document;
}

void ShaderWorkspace::focusDocument(ShaderDocument* document)
{
    if (!entry(document) || m_focused == document)
        return;
    m_focused = document;
    emit focusedDocumentChanged(document);
}

void ShaderWorkspace::bindScene(ShaderDocument* document)
{
    if (!entry(document) || role(document) != ShaderRole::Scene || m_scene == document)
        return;
    m_scene = document;
    emit renderBindingsChanged(m_scene, m_post);
}

void ShaderWorkspace::bindPost(ShaderDocument* document)
{
    if (!entry(document) || role(document) != ShaderRole::Post || m_post == document)
        return;
    m_post = document;
    emit renderBindingsChanged(m_scene, m_post);
}

} // namespace miskeyed::workbench::slang_rhi
