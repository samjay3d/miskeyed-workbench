#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <QFileInfo>
#include <utility>

namespace miskeyed::workbench::slang_rhi {

ShaderWorkspace::ShaderWorkspace(QObject* parent)
    : QObject(parent)
{
}

int ShaderWorkspace::indexOf(ShaderDocument* document) const
{
    for (int i = 0; i < m_sessions.size(); ++i)
        if (m_sessions.at(i).document == document)
            return i;
    return -1;
}

ShaderDocument* ShaderWorkspace::documentAt(int index) const
{
    return index >= 0 && index < m_sessions.size() ? m_sessions.at(index).document : nullptr;
}

DocumentSession* ShaderWorkspace::session(ShaderDocument* document)
{
    const int index = indexOf(document);
    return index >= 0 ? &m_sessions[index] : nullptr;
}

const DocumentSession* ShaderWorkspace::session(ShaderDocument* document) const
{
    const int index = indexOf(document);
    return index >= 0 ? &m_sessions.at(index) : nullptr;
}

QString ShaderWorkspace::displayName(ShaderDocument* document) const
{
    const auto* item = session(document);
    return item ? item->displayName : QString();
}

ShaderDocument* ShaderWorkspace::openSource(
    const QUrl& identity, const QString& displayName, const QString& source)
{
    for (const DocumentSession& item : std::as_const(m_sessions)) {
        if (item.document->fileUrl() == identity) {
            focusDocument(item.document);
            return item.document;
        }
    }

    auto* document = new ShaderDocument(this);
    document->setFileUrl(identity);
    document->setSource(source);
    document->markSourceClean();
    m_sessions.push_back({ document, displayName });
    connect(document, &ShaderDocument::sourceChanged, this,
        [this, document] { emit documentChanged(document); });
    emit documentAdded(document);
    focusDocument(document);
    return document;
}

ShaderDocument* ShaderWorkspace::openFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return nullptr;
    const QUrl identity = QUrl::fromLocalFile(info.absoluteFilePath());
    for (const DocumentSession& item : std::as_const(m_sessions)) {
        if (item.document->fileUrl() == identity) {
            focusDocument(item.document);
            return item.document;
        }
    }
    auto* document = openSource(identity, info.fileName(), QString());
    if (!document->load())
        return nullptr;
    document->compile();
    return document;
}

void ShaderWorkspace::focusDocument(ShaderDocument* document)
{
    if (indexOf(document) < 0 || m_focused == document)
        return;
    emit focusChanging(m_focused, document);
    m_focused = document;
    emit focusedDocumentChanged(document);
}

bool ShaderWorkspace::closeDocument(ShaderDocument* document)
{
    const int index = indexOf(document);
    if (index < 0)
        return false;
    emit documentAboutToClose(document);
    const bool wasFocused = m_focused == document;
    m_sessions.removeAt(index);
    if (wasFocused) {
        m_focused = nullptr;
        if (!m_sessions.isEmpty())
            focusDocument(m_sessions.at(qMin(index, m_sessions.size() - 1)).document);
        else
            emit focusedDocumentChanged(nullptr);
    }
    document->deleteLater();
    emit documentClosed();
    return true;
}

bool ShaderWorkspace::moveDocument(int from, int to)
{
    if (from < 0 || from >= m_sessions.size() || to < 0 || to >= m_sessions.size() || from == to)
        return false;
    m_sessions.move(from, to);
    emit documentOrderChanged();
    return true;
}

} // namespace miskeyed::workbench::slang_rhi
