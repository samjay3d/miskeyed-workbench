#pragma once

#include "Export.h"
#include "LspClient.h"
#include <QMainWindow>
#include <QHash>
#include <QList>
#include <QVariant>

class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QTabWidget;
class QTabBar;
class QStackedWidget;
class QSplitter;
class QTimer;

namespace miskeyed::workbench::core {
class TimeContext;
}

namespace miskeyed::workbench::slang_rhi {

class ShaderDocument;
class SlangRhiWidget;
class CodeEditor;
class LspClient;
class ShaderWorkspace;
class ParameterInspector;

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit WorkbenchWindow(QWidget* parent = nullptr);
    explicit WorkbenchWindow(const QString& shaderPath, QWidget* parent = nullptr);

    // The post-process document/viewport are the primary (public) surface; the scene
    // pass is exposed too so callers can drive the camera programmatically.
    ShaderDocument* document() const { return m_document; }
    ShaderDocument* sceneDocument() const { return m_sceneDocument; }
    SlangRhiWidget* viewport() const { return m_viewport; }
    SlangRhiWidget* sceneViewport() const { return m_sceneViewport; }
    miskeyed::workbench::core::TimeContext* timeContext() const { return m_timeContext; }

    Q_INVOKABLE void openShader(const QString& path);

private:
    void buildUi();
    void connectUi();
    void applyTheme();
    void setupLanguageServer();
    QString documentUri(ShaderDocument* doc) const;
    void setFocusedDocument(ShaderDocument* document);
    void loadSample(const QString& name, int target, const QByteArray& source);
    void updateDocumentTabs();
    void setEditorView(int mode);
    void hookDocument(ShaderDocument* document);
    void reloadGeneratedTargets();
    void refreshGeneratedView();
    void mirrorParameter(ShaderDocument* target, const QString& name, const QVariant& value);

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
    CodeEditor* m_editor = nullptr;
    CodeEditor* m_generatedView = nullptr; // read-only compiled-output viewer
    QPlainTextEdit* m_diagnostics = nullptr;
    QComboBox* m_generatedTarget = nullptr; // HLSL / GLSL / SPIR-V / Metal selector
    ShaderWorkspace* m_workspace = nullptr;
    QTabBar* m_documentTabs = nullptr;
    QWidget* m_sourceSide = nullptr;
    QWidget* m_generatedSide = nullptr;
    QSplitter* m_editorSplit = nullptr;
    QList<QPushButton*> m_viewButtons;
    QPushButton* m_bindDocument = nullptr;
    miskeyed::workbench::core::TimeContext* m_timeContext = nullptr;
    QTimer* m_playbackTimer = nullptr;
    ShaderDocument* m_editorDoc = nullptr; // document currently shown in the editor
    bool m_syncing = false; // guards camera mirroring re-entrancy
    LspClient* m_lsp = nullptr; // Slang language server (slangd)
    QHash<QString, QList<LspDiagnostic>> m_diagnosticsByUri; // last diagnostics per document

    QPushButton* m_compileStatus = nullptr; // persistent compile-state pill
    QTabWidget* m_tabs = nullptr; // Camera / Post-Process / Diagnostics
    ParameterInspector* m_cameraInspector = nullptr;
    ParameterInspector* m_postInspector = nullptr;
    int m_diagTabIndex = -1;
    CompileState m_compileState = CompileState::Idle;
    bool m_lastCompileOk = true;
    int m_editorErrors = 0;
    int m_editorWarnings = 0;
};

} // namespace miskeyed::workbench::slang_rhi
