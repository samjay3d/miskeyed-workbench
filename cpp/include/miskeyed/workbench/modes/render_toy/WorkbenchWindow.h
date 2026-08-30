#pragma once

#include <miskeyed/workbench/Export.h>
#include <miskeyed/workbench/editor/LspClient.h>
#include <QMainWindow>
#include <QHash>
#include <QList>
#include <QVariant>
#include <functional>

class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QTabWidget;
class QTabBar;
class QStackedWidget;
class QSplitter;
class QTimer;
class QTreeWidget;
class QLabel;
class QElapsedTimer;
class QHBoxLayout;

namespace miskeyed::workbench::core {
class TimeContext;
class TimeTransport;
}

namespace miskeyed::workbench::slang_rhi {

class ShaderDocument;
class SlangRhiWidget;
class CodeEditor;
class LspClient;
class ShaderWorkspace;
class ParameterInspector;
class RenderToySession;
class ShaderToySession;
class WorkspaceEditor;
class WorkbenchToolContribution;

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit WorkbenchWindow(QWidget* parent = nullptr);
    explicit WorkbenchWindow(const QString& shaderPath, QWidget* parent = nullptr);
    ~WorkbenchWindow() override;

    // The post-process document/viewport are the primary (public) surface; the scene
    // pass is exposed too so callers can drive the camera programmatically.
    ShaderDocument* document() const { return m_document; }
    ShaderDocument* sceneDocument() const { return m_sceneDocument; }
    ShaderDocument* shaderToyDocument() const;
    SlangRhiWidget* viewport() const { return m_viewport; }
    SlangRhiWidget* sceneViewport() const { return m_sceneViewport; }
    SlangRhiWidget* shaderToyViewport() const { return m_shaderToyViewport; }
    miskeyed::workbench::core::TimeContext* timeContext() const { return m_timeContext; }
    miskeyed::workbench::core::TimeTransport* timeTransport() const { return m_timeTransport; }
    ShaderDocument* focusedDocument() const;
    ParameterInspector* parameterInspector() const { return m_parameterInspector; }
    QString activeTool() const { return m_activeTool; }

    Q_INVOKABLE void openShader(const QString& path);
    Q_INVOKABLE void setActiveTool(const QString& toolId);
    // Tool contributions provide only their mode-specific surface. The stack reparents a
    // registered surface; unregisterTool detaches it without deleting it so its provider
    // can dispose or reuse it. Shared documents, focus, inspector, editor and time stay here.
    Q_INVOKABLE bool registerTool(
        const QString& toolId, const QString& title, QWidget* contributionSurface);
    bool registerToolContribution(WorkbenchToolContribution* contribution);
    Q_INVOKABLE bool unregisterTool(const QString& toolId);
    Q_INVOKABLE void setToolStatus(const QString& toolId, const QString& status);

signals:
    void activeToolChanged(const QString& toolId);

private:
    void buildUi();
    void connectUi();
    void applyTheme();
    void setupLanguageServer();
    QString documentUri(ShaderDocument* doc) const;
    void setFocusedDocument(ShaderDocument* document);
    void loadSample(const QString& name, int target, const QByteArray& source);
    void updateDocumentTabs();
    void hookDocument(ShaderDocument* document);
    void reloadGeneratedTargets();
    void refreshGeneratedView();
    void refreshDependencyInspector();
    void refreshSemanticInspector();
    void mirrorParameter(ShaderDocument* target, const QString& name, const QVariant& value);
    void rebuildToolSelector();
    void setToolSummaryProvider(const QString& toolId, std::function<QString()> provider);

    // Compile-status feedback: a persistent, color-coded pill in the editor bar.
    enum class CompileState { Idle, Dirty, Compiling, Ok, Warn, Error };
    void setCompileState(CompileState state);
    void updateCompileStatus();
    void recountDiagnostics();
    void jumpToFirstError();

    ShaderDocument* m_document = nullptr; // post-process pass
    ShaderDocument* m_sceneDocument = nullptr; // scene / camera pass
    SlangRhiWidget* m_viewport = nullptr; // post-process view
    SlangRhiWidget* m_sceneViewport = nullptr; // scene view
    SlangRhiWidget* m_shaderToyViewport = nullptr; // independent fullscreen consumer
    CodeEditor* m_editor = nullptr;
    CodeEditor* m_generatedView = nullptr; // read-only compiled-output viewer
    WorkspaceEditor* m_workspaceEditor = nullptr;
    QPlainTextEdit* m_diagnostics = nullptr;
    QTreeWidget* m_dependencyTree = nullptr;
    QTreeWidget* m_resourceTree = nullptr;
    QTreeWidget* m_compilationTree = nullptr;
    QPlainTextEdit* m_dependencySource = nullptr;
    QComboBox* m_generatedTarget = nullptr; // HLSL / GLSL / SPIR-V / Metal selector
    ShaderWorkspace* m_workspace = nullptr;
    QStackedWidget* m_toolStack = nullptr;
    QHBoxLayout* m_toolSelectorLayout = nullptr;
    struct ToolContribution final {
        QString id;
        QString title;
        QWidget* surface = nullptr;
        QPushButton* selector = nullptr;
        QString status;
        std::function<QString()> summary;
    };
    QList<ToolContribution> m_toolContributions;
    RenderToySession* m_renderToySession = nullptr;
    ShaderToySession* m_shaderToySession = nullptr;
    miskeyed::workbench::core::TimeContext* m_timeContext = nullptr;
    miskeyed::workbench::core::TimeTransport* m_timeTransport = nullptr;
    QTimer* m_playbackTimer = nullptr;
    QElapsedTimer* m_playbackClock = nullptr;
    bool m_syncing = false; // guards camera mirroring re-entrancy
    LspClient* m_lsp = nullptr; // Slang language server (slangd)
    QHash<QString, QList<LspDiagnostic>> m_diagnosticsByUri; // last diagnostics per document

    QPushButton* m_compileStatus = nullptr; // persistent compile-state pill
    QTabWidget* m_tabs = nullptr; // Active-document semantic inspector
    ParameterInspector* m_parameterInspector = nullptr;
    QLabel* m_inspectorDocument = nullptr;
    QLabel* m_inspectorContext = nullptr;
    QLabel* m_bindingSummary = nullptr;
    int m_diagTabIndex = -1;
    QString m_activeTool;
    CompileState m_compileState = CompileState::Idle;
    bool m_lastCompileOk = true;
    int m_editorErrors = 0;
    int m_editorWarnings = 0;
};

} // namespace miskeyed::workbench::slang_rhi
