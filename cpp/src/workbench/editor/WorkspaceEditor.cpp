#include <miskeyed/workbench/editor/WorkspaceEditor.h>
#include <miskeyed/workbench/editor/CodeEditor.h>
#include <miskeyed/workbench/editor/ShaderHighlighter.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabBar>
#include <QTextCursor>
#include <QVBoxLayout>

namespace miskeyed::workbench::slang_rhi {
namespace {
    QFont editorFont()
    {
        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(11);
        return font;
    }
}

WorkspaceEditor::WorkspaceEditor(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);

    m_tabs = new QTabBar(this);
    m_tabs->setObjectName(QStringLiteral("WorkspaceDocumentTabs"));
    m_tabs->setDocumentMode(true);
    m_tabs->setExpanding(false);
    m_tabs->setMovable(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setElideMode(Qt::ElideMiddle);
    m_tabs->setUsesScrollButtons(true);
    root->addWidget(m_tabs);

    auto* viewBar = new QWidget(this);
    viewBar->setObjectName(QStringLiteral("DocumentViewBar"));
    auto* controls = new QHBoxLayout(viewBar);
    controls->setContentsMargins(8, 3, 8, 3);
    controls->addWidget(new QLabel(QStringLiteral("View"), viewBar));
    for (const QString& label :
        { QStringLiteral("Source"), QStringLiteral("Generated"), QStringLiteral("Compare") }) {
        auto* button = new QPushButton(label, viewBar);
        button->setCheckable(true);
        controls->addWidget(button);
        m_viewButtons.push_back(button);
    }
    controls->addSpacing(8);
    controls->addWidget(new QLabel(QStringLiteral("Target"), viewBar));
    m_generatedTarget = new QComboBox(viewBar);
    m_generatedTarget->setObjectName(QStringLiteral("WorkspaceGeneratedTarget"));
    controls->addWidget(m_generatedTarget);
    controls->addStretch(1);
    root->addWidget(viewBar);

    m_sourceEditor = new CodeEditor(this);
    m_sourceEditor->setObjectName(QStringLiteral("WorkspaceSourceEditor"));
    m_sourceEditor->setFont(editorFont());
    m_sourceEditor->setTabStopDistance(32);
    new ShaderHighlighter(m_sourceEditor->document());
    m_generatedEditor = new CodeEditor(this);
    m_generatedEditor->setObjectName(QStringLiteral("WorkspaceGeneratedEditor"));
    m_generatedEditor->setReadOnly(true);
    m_generatedEditor->setFont(editorFont());
    new ShaderHighlighter(m_generatedEditor->document());

    m_sourceSide = new QWidget(this);
    auto* sourceLayout = new QVBoxLayout(m_sourceSide);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    sourceLayout->setSpacing(0);
    auto* sourceTitle = new QLabel(QStringLiteral("Authored Slang"), m_sourceSide);
    sourceTitle->setObjectName(QStringLiteral("PanelHeader"));
    sourceLayout->addWidget(sourceTitle);
    sourceLayout->addWidget(m_sourceEditor, 1);

    m_generatedSide = new QWidget(this);
    auto* generatedLayout = new QVBoxLayout(m_generatedSide);
    generatedLayout->setContentsMargins(0, 0, 0, 0);
    generatedLayout->setSpacing(0);
    auto* generatedTitle = new QLabel(QStringLiteral("Generated backend output"), m_generatedSide);
    generatedTitle->setObjectName(QStringLiteral("PanelHeader"));
    generatedLayout->addWidget(generatedTitle);
    generatedLayout->addWidget(m_generatedEditor, 1);

    auto* split = new QSplitter(Qt::Horizontal, this);
    split->addWidget(m_sourceSide);
    split->addWidget(m_generatedSide);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    root->addWidget(split, 1);

    connect(m_tabs, &QTabBar::currentChanged, this, [this](int index) {
        if (m_workspace)
            m_workspace->focusDocument(m_workspace->documentAt(index));
    });
    connect(m_tabs, &QTabBar::tabCloseRequested, this, [this](int index) {
        if (!m_workspace)
            return;
        if (ShaderDocument* document = m_workspace->documentAt(index))
            requestClose(document);
    });
    connect(m_tabs, &QTabBar::tabMoved, this, [this](int from, int to) {
        if (m_workspace)
            m_workspace->moveDocument(from, to);
    });
    for (int i = 0; i < m_viewButtons.size(); ++i)
        connect(m_viewButtons.at(i), &QPushButton::clicked, this, [this, i] { setViewMode(i); });
    connect(m_generatedTarget, &QComboBox::currentTextChanged, this, [this](const QString& target) {
        if (!m_workspace)
            return;
        if (DocumentSession* session = m_workspace->session(m_workspace->focusedDocument()))
            session->generatedTarget = target;
        refreshDocument();
    });
    connect(m_sourceEditor, &QPlainTextEdit::textChanged, this, [this] {
        if (!m_workspace || !m_workspace->focusedDocument())
            return;
        ShaderDocument* document = m_workspace->focusedDocument();
        if (document->live())
            document->setSource(m_sourceEditor->toPlainText());
        emit sourceEdited(document, m_sourceEditor->toPlainText());
    });

    auto* closeShortcut = new QShortcut(QKeySequence::Close, this);
    connect(closeShortcut, &QShortcut::activated, this, [this] {
        if (m_workspace)
            requestClose(m_workspace->focusedDocument());
    });
    auto* nextShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), this);
    connect(nextShortcut, &QShortcut::activated, this, [this] { focusRelative(1); });
    auto* previousShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Tab")), this);
    connect(previousShortcut, &QShortcut::activated, this, [this] { focusRelative(-1); });
    setViewMode(0);
}

void WorkspaceEditor::setWorkspace(ShaderWorkspace* workspace)
{
    if (m_workspace == workspace)
        return;
    if (m_workspace)
        disconnect(m_workspace, nullptr, this, nullptr);
    m_workspace = workspace;
    if (!m_workspace)
        return;
    connect(m_workspace, &ShaderWorkspace::focusChanging, this,
        [this](ShaderDocument* previous, ShaderDocument*) { saveSession(previous); });
    connect(m_workspace, &ShaderWorkspace::focusedDocumentChanged, this,
        [this](ShaderDocument* document) {
            m_presented = document;
            restoreSession(document);
            refreshTabs();
            emit focusedDocumentPresented(document);
        });
    connect(m_workspace, &ShaderWorkspace::documentAdded, this, [this](ShaderDocument* document) {
        connect(document, &ShaderDocument::sourceChanged, this, [this, document] {
            if (document == m_workspace->focusedDocument()
                && m_sourceEditor->toPlainText() != document->source()) {
                QSignalBlocker blocker(m_sourceEditor);
                m_sourceEditor->setPlainText(document->source());
            }
            refreshTabs();
        });
        connect(document, &ShaderDocument::dirtyChanged, this, &WorkspaceEditor::refreshTabs);
        connect(document, &ShaderDocument::compiled, this, [this, document] {
            if (document == m_workspace->focusedDocument()) {
                refreshGeneratedTargets();
                refreshDocument();
            }
        });
        refreshTabs();
    });
    connect(m_workspace, &ShaderWorkspace::documentClosed, this, &WorkspaceEditor::refreshTabs);
    connect(
        m_workspace, &ShaderWorkspace::documentOrderChanged, this, &WorkspaceEditor::refreshTabs);
    for (int i = 0; i < m_workspace->documentCount(); ++i) {
        ShaderDocument* document = m_workspace->documentAt(i);
        connect(document, &ShaderDocument::sourceChanged, this, [this, document] {
            if (document == m_workspace->focusedDocument()
                && m_sourceEditor->toPlainText() != document->source()) {
                QSignalBlocker blocker(m_sourceEditor);
                m_sourceEditor->setPlainText(document->source());
            }
            refreshTabs();
        });
        connect(document, &ShaderDocument::dirtyChanged, this, &WorkspaceEditor::refreshTabs);
        connect(document, &ShaderDocument::compiled, this, [this, document] {
            if (document == m_workspace->focusedDocument()) {
                refreshGeneratedTargets();
                refreshDocument();
            }
        });
    }
    m_presented = m_workspace->focusedDocument();
    restoreSession(m_presented);
    refreshTabs();
}

QString WorkspaceEditor::sourceText() const
{
    return m_sourceEditor->toPlainText();
}
QString WorkspaceEditor::generatedText() const
{
    return m_generatedEditor->toPlainText();
}
QString WorkspaceEditor::generatedTarget() const
{
    return m_generatedTarget->currentText();
}

void WorkspaceEditor::setViewMode(int mode)
{
    mode = qBound(0, mode, 2);
    m_sourceSide->setVisible(mode != 1);
    m_generatedSide->setVisible(mode != 0);
    for (int i = 0; i < m_viewButtons.size(); ++i)
        m_viewButtons.at(i)->setChecked(i == mode);
    if (m_workspace)
        if (DocumentSession* session = m_workspace->session(m_workspace->focusedDocument()))
            session->viewMode = mode;
}

int WorkspaceEditor::viewMode() const
{
    for (int i = 0; i < m_viewButtons.size(); ++i)
        if (m_viewButtons.at(i)->isChecked())
            return i;
    return 0;
}

void WorkspaceEditor::saveSession(ShaderDocument* document)
{
    if (!m_workspace || !document)
        return;
    DocumentSession* session = m_workspace->session(document);
    if (!session)
        return;
    const QTextCursor cursor = m_sourceEditor->textCursor();
    session->cursorPosition = cursor.position();
    session->anchorPosition = cursor.anchor();
    session->verticalScroll = m_sourceEditor->verticalScrollBar()->value();
    session->horizontalScroll = m_sourceEditor->horizontalScrollBar()->value();
    session->viewMode = viewMode();
    session->generatedTarget = m_generatedTarget->currentText();
}

void WorkspaceEditor::restoreSession(ShaderDocument* document)
{
    QSignalBlocker sourceBlock(m_sourceEditor);
    if (!document) {
        m_sourceEditor->clear();
        m_generatedEditor->clear();
        return;
    }
    m_sourceEditor->setPlainText(document->source());
    refreshGeneratedTargets();
    const DocumentSession* session = m_workspace->session(document);
    if (!session)
        return;
    setViewMode(session->viewMode);
    const int target = m_generatedTarget->findText(session->generatedTarget);
    if (target >= 0)
        m_generatedTarget->setCurrentIndex(target);
    QTextCursor cursor = m_sourceEditor->textCursor();
    const int last = qMax(0, m_sourceEditor->document()->characterCount() - 1);
    cursor.setPosition(qBound(0, session->anchorPosition, last));
    cursor.setPosition(qBound(0, session->cursorPosition, last), QTextCursor::KeepAnchor);
    m_sourceEditor->setTextCursor(cursor);
    m_sourceEditor->verticalScrollBar()->setValue(session->verticalScroll);
    m_sourceEditor->horizontalScrollBar()->setValue(session->horizontalScroll);
    refreshDocument();
}

void WorkspaceEditor::refreshTabs()
{
    if (!m_workspace)
        return;
    QSignalBlocker blocker(m_tabs);
    while (m_tabs->count() < m_workspace->documentCount())
        m_tabs->addTab(QString());
    while (m_tabs->count() > m_workspace->documentCount())
        m_tabs->removeTab(m_tabs->count() - 1);
    for (int i = 0; i < m_workspace->documentCount(); ++i) {
        ShaderDocument* document = m_workspace->documentAt(i);
        m_tabs->setTabText(i,
            m_workspace->displayName(document)
                + (document->dirty() ? QStringLiteral(" •") : QString()));
        m_tabs->setTabToolTip(i, document->fileUrl().toString());
        if (document == m_workspace->focusedDocument())
            m_tabs->setCurrentIndex(i);
    }
}

void WorkspaceEditor::refreshGeneratedTargets()
{
    if (!m_workspace || !m_workspace->focusedDocument())
        return;
    const QString current = m_generatedTarget->currentText();
    QSignalBlocker blocker(m_generatedTarget);
    m_generatedTarget->clear();
    m_generatedTarget->addItems(m_workspace->focusedDocument()->generatedTargets());
    const int index = m_generatedTarget->findText(current);
    if (index >= 0)
        m_generatedTarget->setCurrentIndex(index);
}

void WorkspaceEditor::refreshDocument()
{
    if (!m_workspace || !m_workspace->focusedDocument()) {
        m_generatedEditor->clear();
        return;
    }
    m_generatedEditor->setPlainText(
        m_workspace->focusedDocument()->generatedCode(m_generatedTarget->currentText()));
}

void WorkspaceEditor::focusRelative(int offset)
{
    if (!m_workspace || m_workspace->documentCount() < 2)
        return;
    int current = 0;
    for (int i = 0; i < m_workspace->documentCount(); ++i)
        if (m_workspace->documentAt(i) == m_workspace->focusedDocument())
            current = i;
    const int next
        = (current + offset + m_workspace->documentCount()) % m_workspace->documentCount();
    m_workspace->focusDocument(m_workspace->documentAt(next));
}

void WorkspaceEditor::requestClose(ShaderDocument* document)
{
    if (!m_workspace || !document)
        return;
    if (document->dirty()) {
        const auto choice = QMessageBox::warning(this, QStringLiteral("Unsaved shader"),
            QStringLiteral("Save changes to %1 before closing?")
                .arg(m_workspace->displayName(document)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (choice == QMessageBox::Cancel)
            return;
        if (choice == QMessageBox::Save && !document->save())
            return;
    }
    m_workspace->closeDocument(document);
}

} // namespace miskeyed::workbench::slang_rhi
