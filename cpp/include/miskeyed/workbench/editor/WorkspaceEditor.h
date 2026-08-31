#pragma once

#include <miskeyed/workbench/Export.h>
#include <QList>
#include <QWidget>

class QComboBox;
class QTabBar;
class QSplitter;
class QShortcut;
class QPushButton;

namespace miskeyed::workbench::slang_rhi {
class CodeEditor;
class ShaderDocument;
class ShaderWorkspace;

// VS Code-style document editor group over ShaderWorkspace. It owns document tabs and
// per-document view-session binding, while ShaderDocument owns authored/compiler state.
// Runtime mode bindings deliberately remain outside this reusable component.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkspaceEditor final : public QWidget {
    Q_OBJECT
public:
    explicit WorkspaceEditor(QWidget* parent = nullptr);

    void setWorkspace(ShaderWorkspace* workspace);
    ShaderWorkspace* workspace() const { return m_workspace; }
    CodeEditor* sourceEditor() const { return m_sourceEditor; }
    CodeEditor* generatedEditor() const { return m_generatedEditor; }
    QComboBox* targetSelector() const { return m_generatedTarget; }
    QString sourceText() const;
    QString generatedText() const;
    QString generatedTarget() const;
    void setViewMode(int mode);
    int viewMode() const;
    void refreshDocument();

signals:
    void sourceEdited(ShaderDocument* document, QString source);
    void focusedDocumentPresented(ShaderDocument* document);

private:
    void saveSession(ShaderDocument* document);
    void restoreSession(ShaderDocument* document);
    void refreshTabs();
    void refreshGeneratedTargets();
    void focusRelative(int offset);
    void requestClose(ShaderDocument* document);

    ShaderWorkspace* m_workspace = nullptr;
    QTabBar* m_tabs = nullptr;
    CodeEditor* m_sourceEditor = nullptr;
    CodeEditor* m_generatedEditor = nullptr;
    QComboBox* m_generatedTarget = nullptr;
    QWidget* m_sourceSide = nullptr;
    QWidget* m_generatedSide = nullptr;
    QList<QPushButton*> m_viewButtons;
};

} // namespace miskeyed::workbench::slang_rhi
