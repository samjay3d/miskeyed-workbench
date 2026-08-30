#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <miskeyed/workbench/modes/render_toy/WorkbenchWindow.h>
#include <miskeyed/workbench/ui/WorkbenchTheme.h>
#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/ui/ParameterInspector.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/editor/CodeEditor.h>
#include <miskeyed/workbench/editor/LspClient.h>
#include <miskeyed/workbench/editor/WorkspaceEditor.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <miskeyed/workbench/core/ViewportCamera.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QSysInfo>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace miskeyed::workbench::slang_rhi {

namespace {

    // A good fixed-width coding font if available, falling back to the platform default.
    QFont monospaceFont(int pt = 11)
    {
        for (const QString& family :
            { QStringLiteral("Cascadia Code"), QStringLiteral("JetBrains Mono"),
                QStringLiteral("Consolas"), QStringLiteral("Menlo") }) {
            if (QFontDatabase::families().contains(family)) {
                QFont f(family, pt);
                f.setStyleHint(QFont::Monospace);
                return f;
            }
        }
        QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        f.setPointSize(pt);
        return f;
    }

    QByteArray readShaderResource(const char* path)
    {
        QFile file(QString::fromUtf8(path));
        if (!file.open(QIODevice::ReadOnly))
            return QByteArray("// Workbench could not load the embedded shader sample.\n");
        return file.readAll();
    }

    QByteArray renderToySource(const char* path)
    {
        return readShaderResource(path);
    }

    QByteArray sceneShaderSource()
    {
        return renderToySource(":/miskeyed/workbench/render_toy/scene_default.slang");
    }

    QByteArray postShaderSource()
    {
        return renderToySource(":/miskeyed/workbench/render_toy/post_default.slang");
    }

    struct SampleShader {
        const char* title;
        int target;
        const char* resource;
    };

    const std::array kSampleShaders = {
        SampleShader {
            "Scene — Default studio", 0, ":/miskeyed/workbench/render_toy/scene_default.slang" },
        SampleShader {
            "Scene — Raymarched clouds", 0, ":/miskeyed/workbench/render_toy/scene_clouds.slang" },
        SampleShader {
            "Post — Default grade", 1, ":/miskeyed/workbench/render_toy/post_default.slang" },
        SampleShader { "Post — Bloom + chromatic aberration", 1,
            ":/miskeyed/workbench/render_toy/post_bloom.slang" },
        SampleShader {
            "Post — CRT / scanlines", 1, ":/miskeyed/workbench/render_toy/post_crt.slang" },
    };

} // namespace

WorkbenchWindow::WorkbenchWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    connectUi();
    updateDocumentTabs();
    setupLanguageServer();
    setFocusedDocument(m_document);
    m_sceneDocument->compile();
    m_document->compile();
}
WorkbenchWindow::WorkbenchWindow(const QString& shaderPath, QWidget* parent)
    : WorkbenchWindow(parent)
{
    if (!shaderPath.isEmpty())
        openShader(shaderPath);
}

WorkbenchWindow::~WorkbenchWindow()
{
    delete m_playbackClock;
}

ShaderDocument* WorkbenchWindow::focusedDocument() const
{
    return m_workspace ? m_workspace->focusedDocument() : nullptr;
}

void WorkbenchWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Workbench"));
    resize(1600, 950);
    m_renderToySession = new RenderToySession(this);
    m_timeContext = m_renderToySession->timeContext();
    m_timeTransport = m_renderToySession->timeTransport();
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setTimerType(Qt::PreciseTimer);
    m_playbackClock = new QElapsedTimer;
    m_workspace = new ShaderWorkspace(this);
    m_sceneDocument
        = m_workspace->openSource(QUrl(QStringLiteral("workbench:/samples/scene_default.slang")),
            QStringLiteral("scene_default.slang"), QString::fromUtf8(sceneShaderSource()));
    m_document
        = m_workspace->openSource(QUrl(QStringLiteral("workbench:/samples/post_default.slang")),
            QStringLiteral("post_default.slang"), QString::fromUtf8(postShaderSource()));
    m_renderToySession->bindScene(m_sceneDocument);
    m_renderToySession->bindPost(m_document);

    m_sceneViewport = new SlangRhiWidget(this);
    m_sceneViewport->setObjectName(QStringLiteral("SceneViewport"));
    m_sceneViewport->setTimeContext(m_timeContext);
    m_sceneViewport->setDocument(m_sceneDocument);
    m_viewport = new SlangRhiWidget(this);
    m_viewport->setObjectName(QStringLiteral("PostViewport"));
    m_viewport->setTimeContext(m_timeContext);
    m_viewport->setDocument(m_document);
    // The post viewport runs a real two-pass pipeline: it renders the scene document into
    // an offscreen texture (G-buffer), then its own document grades that texture on top.
    m_viewport->setScenePass(m_sceneDocument);
    m_workspaceEditor = new WorkspaceEditor(this);
    m_workspaceEditor->setWorkspace(m_workspace);
    m_editor = m_workspaceEditor->sourceEditor();
    m_generatedView = m_workspaceEditor->generatedEditor();
    m_generatedTarget = m_workspaceEditor->targetSelector();

    m_parameterInspector = new ParameterInspector(this);
    m_parameterInspector->setObjectName(QStringLiteral("ActiveDocumentParameters"));
    m_diagnostics = new QPlainTextEdit(this);
    m_diagnostics->setReadOnly(true);
    m_diagnostics->setFont(monospaceFont(10));
    m_diagnostics->setPlaceholderText(QStringLiteral("No diagnostics — shader compiled cleanly."));
    auto* dependencyPanel = new QSplitter(Qt::Vertical, this);
    m_dependencyTree = new QTreeWidget(dependencyPanel);
    m_dependencyTree->setHeaderLabels({ QStringLiteral("Kind"), QStringLiteral("Identity / path"),
        QStringLiteral("Hash"), QStringLiteral("State") });
    m_dependencyTree->header()->setStretchLastSection(false);
    m_dependencyTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_dependencySource = new QPlainTextEdit(dependencyPanel);
    m_dependencySource->setReadOnly(true);
    m_dependencySource->setFont(monospaceFont(10));
    m_dependencySource->setPlaceholderText(
        QStringLiteral("Select an imported module to inspect its resolved source."));
    dependencyPanel->addWidget(m_dependencyTree);
    dependencyPanel->addWidget(m_dependencySource);
    dependencyPanel->setSizes({ 220, 180 });

    auto viewportPanel = [this](const QString& slot, QComboBox*& binding, QWidget* w) {
        auto* box = new QWidget(this);
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
        auto* header = new QWidget(box);
        header->setObjectName(QStringLiteral("PanelHeader"));
        auto* h = new QHBoxLayout(header);
        h->setContentsMargins(8, 3, 8, 3);
        auto* lbl = new QLabel(slot, header);
        lbl->setObjectName(QStringLiteral("PanelHeaderInline"));
        h->addWidget(lbl);
        binding = new QComboBox(header);
        binding->setToolTip(QStringLiteral("Document bound to the %1 runtime slot").arg(slot));
        h->addWidget(binding, 1);
        h->addWidget(new QLabel(QStringLiteral("linked"), header));
        v->addWidget(header);
        v->addWidget(w, 1);
        return box;
    };

    auto* views = new QSplitter(Qt::Horizontal, this);
    views->addWidget(viewportPanel(QStringLiteral("Scene"), m_sceneBinding, m_sceneViewport));
    views->addWidget(viewportPanel(QStringLiteral("Post"), m_postBinding, m_viewport));
    views->setStretchFactor(0, 1);
    views->setStretchFactor(1, 1);
    m_sceneViewport->setToolTip(QStringLiteral(
        "Scene pass. Rendered into an offscreen color texture (the G-buffer) that the\n"
        "post-process pass reads. Drag to move the camera (Houdini nav):\n"
        "  Left button  — orbit / tumble\n"
        "  Middle button — pan\n"
        "  Right button / wheel — dolly / zoom\n"
        "Click this viewport to focus the Scene-bound document tab below."));
    m_viewport->setToolTip(QStringLiteral(
        "Post-process pass. This does NOT re-render the scene — it samples the scene\n"
        "G-buffer texture (`sceneColor`) and grades it (exposure / tint / vignette), then\n"
        "draws on top. Click this viewport to focus the Post-bound document tab below.\n"
        "Camera drag here still moves the shared scene camera."));

    auto* inspector = new QWidget(this);
    inspector->setObjectName(QStringLiteral("InspectorPanel"));
    auto* inspectorLayout = new QVBoxLayout(inspector);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);
    inspectorLayout->setSpacing(0);
    auto* inspectorHeader = new QWidget(inspector);
    inspectorHeader->setObjectName(QStringLiteral("InspectorHeader"));
    auto* inspectorHeaderLayout = new QVBoxLayout(inspectorHeader);
    inspectorHeaderLayout->setContentsMargins(10, 7, 10, 7);
    inspectorHeaderLayout->setSpacing(2);
    m_inspectorDocument = new QLabel(inspectorHeader);
    m_inspectorDocument->setObjectName(QStringLiteral("InspectorDocument"));
    m_inspectorContext = new QLabel(inspectorHeader);
    m_inspectorContext->setObjectName(QStringLiteral("InspectorContext"));
    inspectorHeaderLayout->addWidget(m_inspectorDocument);
    inspectorHeaderLayout->addWidget(m_inspectorContext);
    inspectorLayout->addWidget(inspectorHeader);

    m_resourceTree = new QTreeWidget(inspector);
    m_resourceTree->setHeaderLabels({ QStringLiteral("Resource"), QStringLiteral("Kind"),
        QStringLiteral("Binding"), QStringLiteral("Space") });
    m_resourceTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    auto* compilationPanel = new QSplitter(Qt::Vertical, inspector);
    m_compilationTree = new QTreeWidget(compilationPanel);
    m_compilationTree->setHeaderLabels({ QStringLiteral("Compilation"), QStringLiteral("Value") });
    m_compilationTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    compilationPanel->addWidget(m_compilationTree);
    compilationPanel->addWidget(m_diagnostics);
    compilationPanel->setSizes({ 230, 180 });

    auto* tabs = new QTabWidget(inspector);
    tabs->setObjectName(QStringLiteral("ActiveDocumentInspector"));
    tabs->addTab(m_parameterInspector, QStringLiteral("Parameters"));
    tabs->addTab(m_resourceTree, QStringLiteral("Resources"));
    // Reflection belongs in the inspector. Source and generated text are editor views,
    // not permanent inspector panels.
    tabs->addTab(dependencyPanel, QStringLiteral("Dependencies"));
    m_diagTabIndex = tabs->addTab(compilationPanel, QStringLiteral("Compilation"));
    m_tabs = tabs;
    tabs->setMinimumWidth(380);
    inspectorLayout->addWidget(tabs, 1);

    auto* editorBox = new QWidget(this);
    auto* ev = new QVBoxLayout(editorBox);
    ev->setContentsMargins(0, 0, 0, 0);
    ev->setSpacing(2);

    auto* timeline = new QWidget(editorBox);
    timeline->setObjectName(QStringLiteral("Timeline"));
    auto* th = new QHBoxLayout(timeline);
    th->setContentsMargins(8, 3, 8, 3);
    auto transportButton = [timeline, th](const QString& text, const QString& tip) {
        auto* button = new QPushButton(text, timeline);
        button->setToolTip(tip);
        button->setFixedWidth(34);
        th->addWidget(button);
        return button;
    };
    auto* firstFrame = transportButton(QStringLiteral("|<"), QStringLiteral("First frame"));
    auto* previousFrame = transportButton(QStringLiteral("<"), QStringLiteral("Previous frame"));
    auto* playPause = transportButton(QStringLiteral("▶"), QStringLiteral("Play / pause"));
    auto* nextFrame = transportButton(QStringLiteral(">"), QStringLiteral("Next frame"));
    auto* lastFrame = transportButton(QStringLiteral(">|"), QStringLiteral("Last frame"));
    auto* frameSpin = new QDoubleSpinBox(timeline);
    auto* startSpin = new QDoubleSpinBox(timeline);
    auto* endSpin = new QDoubleSpinBox(timeline);
    auto* fpsSpin = new QDoubleSpinBox(timeline);
    auto* timeLabel = new QLabel(timeline);
    auto* frameSlider = new QSlider(Qt::Horizontal, timeline);
    for (QDoubleSpinBox* spin : { frameSpin, startSpin, endSpin }) {
        spin->setRange(-1000000, 1000000);
        spin->setDecimals(3);
        spin->setSingleStep(1.0);
    }
    auto syncTransportRange = [this, frameSpin, frameSlider, startSpin, endSpin, fpsSpin] {
        QSignalBlocker frameBlock(frameSpin);
        QSignalBlocker sliderBlock(frameSlider);
        QSignalBlocker startBlock(startSpin);
        QSignalBlocker endBlock(endSpin);
        QSignalBlocker fpsBlock(fpsSpin);
        const double start = m_timeTransport->startValue();
        const double end = m_timeTransport->endValue();
        frameSpin->setRange(start, end);
        frameSlider->setRange(qFloor(start), qCeil(end));
        startSpin->setValue(start);
        endSpin->setValue(end);
        fpsSpin->setValue(m_timeTransport->rate());
    };
    fpsSpin->setRange(0.001, 1000.0);
    fpsSpin->setDecimals(3);
    syncTransportRange();
    th->addWidget(new QLabel(QStringLiteral("Frame"), timeline));
    th->addWidget(frameSpin);
    th->addWidget(timeLabel);
    th->addWidget(new QLabel(QStringLiteral("FPS"), timeline));
    th->addWidget(fpsSpin);
    th->addWidget(new QLabel(QStringLiteral("Range"), timeline));
    th->addWidget(startSpin);
    th->addWidget(new QLabel(QStringLiteral("to"), timeline));
    th->addWidget(endSpin);
    th->addWidget(frameSlider, 1);

    connect(firstFrame, &QPushButton::clicked, this, [this] {
        m_timeTransport->seek(
            core::TimeValue(m_timeTransport->startValue(), m_timeTransport->rate()));
    });
    connect(previousFrame, &QPushButton::clicked, this, [this] { m_timeTransport->step(-1.0); });
    connect(nextFrame, &QPushButton::clicked, this, [this] { m_timeTransport->step(1.0); });
    connect(lastFrame, &QPushButton::clicked, this, [this] {
        m_timeTransport->seek(
            core::TimeValue(m_timeTransport->endValue(), m_timeTransport->rate()));
    });
    connect(playPause, &QPushButton::clicked, this,
        [this] { m_timeTransport->setPlaying(!m_timeTransport->playing()); });
    connect(frameSpin, &QDoubleSpinBox::valueChanged, this, [this](double frame) {
        m_timeTransport->seek(core::TimeValue(frame, m_timeTransport->rate()));
    });
    connect(frameSlider, &QSlider::valueChanged, this, [this](int frame) {
        m_timeTransport->seek(core::TimeValue(double(frame), m_timeTransport->rate()));
    });
    connect(fpsSpin, &QDoubleSpinBox::valueChanged, m_timeTransport, &core::TimeTransport::setRate);
    auto setRange = [this, startSpin, endSpin] {
        m_timeTransport->setPlaybackRange(
            core::TimeValue(startSpin->value(), m_timeTransport->rate()),
            core::TimeValue(endSpin->value(), m_timeTransport->rate()));
    };
    connect(startSpin, &QDoubleSpinBox::valueChanged, this, setRange);
    connect(endSpin, &QDoubleSpinBox::valueChanged, this, setRange);
    connect(m_timeTransport, &core::TimeTransport::playingChanged, this,
        [this, playPause](bool playing) {
            playPause->setText(playing ? QStringLiteral("❚❚") : QStringLiteral("▶"));
            if (playing) {
                m_playbackClock->restart();
                m_playbackTimer->start(qMax(1, qRound(1000.0 / m_timeTransport->rate())));
            } else {
                m_playbackTimer->stop();
            }
        });
    connect(m_playbackTimer, &QTimer::timeout, this, [this] {
        const double elapsedSeconds = double(m_playbackClock->restart()) / 1000.0;
        m_timeTransport->advanceSeconds(elapsedSeconds);
    });
    connect(m_timeTransport, &core::TimeTransport::playbackChanged, this, syncTransportRange);
    connect(m_timeContext, &core::TimeContext::changed, this,
        [this, frameSpin, frameSlider, timeLabel] {
            QSignalBlocker frameBlock(frameSpin);
            QSignalBlocker sliderBlock(frameSlider);
            frameSpin->setValue(m_timeContext->timeValue());
            frameSlider->setValue(qRound(m_timeContext->timeValue()));
            timeLabel->setText(
                QStringLiteral("Time %1 s").arg(m_timeContext->timeSeconds(), 0, 'f', 3));
            if (m_timeTransport->playing())
                m_playbackTimer->setInterval(qMax(1, qRound(1000.0 / m_timeTransport->rate())));
        });
    timeLabel->setText(QStringLiteral("Time 0.000 s"));
    auto* modeBar = new QWidget(editorBox);
    modeBar->setObjectName(QStringLiteral("ModeActionBar"));
    auto* modeActions = new QHBoxLayout(modeBar);
    modeActions->setContentsMargins(8, 3, 8, 3);
    modeActions->addWidget(new QLabel(QStringLiteral("Render Toy"), modeBar));
    m_bindScene = new QPushButton(QStringLiteral("Use as Scene"), modeBar);
    m_bindPost = new QPushButton(QStringLiteral("Use as Post"), modeBar);
    m_bindScene->setToolTip(
        QStringLiteral("Explicitly bind the focused document to the Scene runtime slot."));
    m_bindPost->setToolTip(
        QStringLiteral("Explicitly bind the focused document to the Post runtime slot."));
    modeActions->addWidget(m_bindScene);
    modeActions->addWidget(m_bindPost);

    m_compileStatus = new QPushButton(modeBar);
    m_compileStatus->setObjectName(QStringLiteral("CompileStatus"));
    m_compileStatus->setCursor(Qt::PointingHandCursor);
    m_compileStatus->setFocusPolicy(Qt::NoFocus);
    modeActions->addWidget(m_compileStatus);
    modeActions->addStretch(1);
    auto* navHint = new QLabel(
        QStringLiteral("Camera: drag = orbit · middle = pan · right/wheel = zoom"), modeBar);
    navHint->setObjectName(QStringLiteral("HintLabel"));
    modeActions->addWidget(navHint);

    ev->addWidget(modeBar);
    ev->addWidget(m_workspaceEditor, 1);
    // Transport is a persistent controller for the active Render Toy evaluation,
    // independent of whichever authoring document currently has focus.
    ev->addWidget(timeline);

    auto* documentWorkspace = new QSplitter(Qt::Vertical, this);
    documentWorkspace->addWidget(views);
    documentWorkspace->addWidget(editorBox);
    documentWorkspace->setStretchFactor(0, 3);
    documentWorkspace->setStretchFactor(1, 2);
    documentWorkspace->setSizes({ 520, 380 });

    // The inspector spans result and editor surfaces because it describes the focused
    // document, not either viewport or either Render Toy binding.
    auto* root = new QSplitter(Qt::Horizontal, this);
    root->addWidget(documentWorkspace);
    root->addWidget(inspector);
    root->setStretchFactor(0, 4);
    root->setStretchFactor(1, 1);
    root->setSizes({ 1220, 380 });
    setCentralWidget(root);
    setStatusBar(new QStatusBar(this));
    m_bindingSummary = new QLabel(statusBar());
    m_bindingSummary->setObjectName(QStringLiteral("BindingSummary"));
    statusBar()->addWidget(m_bindingSummary, 1);

    auto* tb = addToolBar(QStringLiteral("Shader"));
    tb->setMovable(false);
    auto* open = tb->addAction(QStringLiteral("Open"));
    open->setShortcut(QKeySequence::Open);
    auto* save = tb->addAction(QStringLiteral("Save"));
    save->setShortcut(QKeySequence::Save);
    auto* compile = tb->addAction(QStringLiteral("Compile"));
    compile->setShortcut(QKeySequence(QStringLiteral("Ctrl+Return")));
    auto* live = tb->addAction(QStringLiteral("Live"));
    live->setCheckable(true);
    live->setChecked(true);
    tb->addSeparator();
    auto* exportOut = tb->addAction(QStringLiteral("Export output…"));
    exportOut->setToolTip(QStringLiteral(
        "Save the currently shown compiled output (HLSL/GLSL/SPIR-V/Metal) to a file."));

    tb->addSeparator();
    auto* samplesBtn = new QToolButton(tb);
    samplesBtn->setText(QStringLiteral("Samples"));
    samplesBtn->setPopupMode(QToolButton::InstantPopup);
    samplesBtn->setToolTip(QStringLiteral(
        "Load a ready-made sample: a camera-driven volume raymarch or a post-process effect."));
    auto* samplesMenu = new QMenu(samplesBtn);
    for (const SampleShader& sample : kSampleShaders) {
        QAction* action = samplesMenu->addAction(QString::fromUtf8(sample.title));
        const int target = sample.target;
        const auto* resource = sample.resource;
        const QString name = QFileInfo(QString::fromUtf8(resource)).fileName();
        connect(action, &QAction::triggered, this, [this, name, target, resource] {
            loadSample(name, target, renderToySource(resource));
        });
    }
    samplesBtn->setMenu(samplesMenu);
    tb->addWidget(samplesBtn);

    connect(open, &QAction::triggered, this, [this] {
        const auto p = QFileDialog::getOpenFileName(
            this, QStringLiteral("Open Slang shader"), {}, QStringLiteral("Slang (*.slang)"));
        if (!p.isEmpty())
            openShader(p);
    });
    connect(save, &QAction::triggered, this, [this] {
        if (m_workspace->focusedDocument()) {
            m_workspace->focusedDocument()->setSource(m_editor->toPlainText());
            m_workspace->focusedDocument()->save();
        }
    });
    connect(compile, &QAction::triggered, this, [this] {
        if (m_workspace->focusedDocument()) {
            m_workspace->focusedDocument()->setSource(m_editor->toPlainText());
            m_workspace->focusedDocument()->compile();
        }
    });
    connect(live, &QAction::toggled, this, [this](bool on) {
        for (int i = 0; i < m_workspace->documentCount(); ++i)
            m_workspace->documentAt(i)->setLive(on);
    });
    connect(exportOut, &QAction::triggered, this, [this] {
        if (!m_generatedView || m_generatedView->toPlainText().isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Nothing to export — compile first"), 1800);
            return;
        }
        const QString target
            = m_generatedTarget ? m_generatedTarget->currentText() : QStringLiteral("output");
        static const QMap<QString, QString> ext
            = { { QStringLiteral("HLSL"), QStringLiteral("hlsl") },
                  { QStringLiteral("GLSL"), QStringLiteral("glsl") },
                  { QStringLiteral("SPIR-V"), QStringLiteral("spvasm") },
                  { QStringLiteral("Metal"), QStringLiteral("metal") } };
        const QString suggested
            = QStringLiteral("shader.%1").arg(ext.value(target, QStringLiteral("txt")));
        const QString p = QFileDialog::getSaveFileName(
            this, QStringLiteral("Export compiled %1").arg(target), suggested);
        if (p.isEmpty())
            return;
        QFile f(p);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(m_generatedView->toPlainText().toUtf8());
            statusBar()->showMessage(QStringLiteral("Exported %1 to %2").arg(target, p), 2400);
        }
    });

    applyTheme();
}

void WorkbenchWindow::connectUi()
{
    connect(m_workspaceEditor, &WorkspaceEditor::sourceEdited, this,
        [this](ShaderDocument*, const QString&) { setCompileState(CompileState::Dirty); });

    connect(m_compileStatus, &QPushButton::clicked, this, [this] {
        if (m_editorErrors > 0 || (!m_lastCompileOk && m_compileState == CompileState::Error))
            jumpToFirstError();
        else if (m_workspace->focusedDocument()) {
            m_workspace->focusedDocument()->setSource(m_editor->toPlainText());
            m_workspace->focusedDocument()->compile();
        }
    });

    hookDocument(m_sceneDocument);
    hookDocument(m_document);
    connect(m_workspace, &ShaderWorkspace::documentAdded, this, [this](ShaderDocument* doc) {
        hookDocument(doc);
        updateDocumentTabs();
        if (m_lsp)
            m_lsp->openDocument(documentUri(doc), doc->source());
    });
    connect(m_workspace, &ShaderWorkspace::documentChanged, this,
        [this](ShaderDocument*) { updateDocumentTabs(); });
    connect(m_workspace, &ShaderWorkspace::focusedDocumentChanged, this,
        [this](ShaderDocument* doc) { setFocusedDocument(doc); });
    connect(m_renderToySession, &RenderToySession::bindingsChanged, this,
        [this](ShaderDocument* scene, ShaderDocument* post) {
            m_sceneDocument = scene;
            m_document = post;
            m_sceneViewport->setDocument(scene);
            m_viewport->setScenePass(scene);
            m_viewport->setDocument(post);
            // Binding changes affect rendering only. The inspector continues to follow
            // workspace focus, even when another document is bound behind the scenes.
            if (m_workspace->focusedDocument())
                setFocusedDocument(m_workspace->focusedDocument());
            updateDocumentTabs();
        });
    connect(m_workspace, &ShaderWorkspace::documentAboutToClose, this,
        [this](ShaderDocument* document) { m_renderToySession->removeDocument(document); });
    connect(m_workspace, &ShaderWorkspace::documentClosed, this, [this] { updateDocumentTabs(); });
    connect(m_workspace, &ShaderWorkspace::documentOrderChanged, this,
        [this] { updateDocumentTabs(); });

    connect(m_sceneViewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });
    connect(m_viewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });

    connect(m_bindScene, &QPushButton::clicked, this,
        [this] { m_renderToySession->bindScene(m_workspace->focusedDocument()); });
    connect(m_bindPost, &QPushButton::clicked, this,
        [this] { m_renderToySession->bindPost(m_workspace->focusedDocument()); });
    connect(m_sceneBinding, QOverload<int>::of(&QComboBox::activated), this,
        [this](int index) { m_renderToySession->bindScene(m_workspace->documentAt(index)); });
    connect(m_postBinding, QOverload<int>::of(&QComboBox::activated), this,
        [this](int index) { m_renderToySession->bindPost(m_workspace->documentAt(index)); });
    connect(
        m_dependencyTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
            if (!item || !m_workspace->focusedDocument()) {
                m_dependencySource->clear();
                return;
            }
            const NodeId node = item->data(0, Qt::UserRole).toULongLong();
            m_dependencySource->setPlainText(QString::fromUtf8(
                m_workspace->focusedDocument()->dependencyGraph()->payload(node)));
        });
    connect(m_sceneViewport, &SlangRhiWidget::activated, this,
        [this] { m_workspace->focusDocument(m_renderToySession->sceneDocument()); });
    connect(m_viewport, &SlangRhiWidget::activated, this,
        [this] { m_workspace->focusDocument(m_renderToySession->postDocument()); });
}

void WorkbenchWindow::hookDocument(ShaderDocument* doc)
{
    connect(doc, &ShaderDocument::dirtyChanged, this, [this] { updateDocumentTabs(); });
    connect(doc, &ShaderDocument::diagnosticsChanged, this, [this, doc] {
        if (doc == m_workspace->focusedDocument())
            m_diagnostics->setPlainText(doc->diagnostics());
    });
    connect(doc, &ShaderDocument::dependenciesChanged, this, [this, doc] {
        if (doc == m_workspace->focusedDocument())
            refreshDependencyInspector();
    });
    connect(doc->dependencyGraph(), &DependencyGraph::graphChanged, this, [this, doc] {
        if (doc == m_workspace->focusedDocument())
            refreshDependencyInspector();
    });
    connect(
        doc->dependencyGraph(), &DependencyGraph::nodeChanged, this, [this, doc](quint64, quint32) {
            if (doc == m_workspace->focusedDocument())
                refreshDependencyInspector();
        });
    connect(doc, &ShaderDocument::compilingChanged, this, [this, doc] {
        if (doc == m_workspace->focusedDocument() && doc->compiling()) {
            setCompileState(CompileState::Compiling);
            m_compileStatus->repaint();
        }
    });
    connect(doc, &ShaderDocument::compiled, this, [this, doc] {
        if (doc != m_workspace->focusedDocument())
            return;
        m_lastCompileOk = true;
        recountDiagnostics();
        setCompileState(m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok);
        reloadGeneratedTargets();
        refreshSemanticInspector();
    });
    connect(doc, &ShaderDocument::compileFailed, this, [this, doc](const QString&) {
        if (doc != m_workspace->focusedDocument())
            return;
        m_lastCompileOk = false;
        recountDiagnostics();
        setCompileState(CompileState::Error);
        refreshSemanticInspector();
    });
    connect(doc->parameters(), &ShaderParameterModel::parameterChanged, this,
        [this, doc](const QString& name, const QVariant& value, int, int) {
            if (doc == m_renderToySession->sceneDocument())
                mirrorParameter(m_renderToySession->postDocument(), name, value);
            else if (doc == m_renderToySession->postDocument())
                mirrorParameter(m_renderToySession->sceneDocument(), name, value);
        });
}

void WorkbenchWindow::setFocusedDocument(ShaderDocument* document)
{
    if (!document) {
        m_parameterInspector->setModel(nullptr);
        m_diagnostics->clear();
        m_dependencyTree->clear();
        m_resourceTree->clear();
        m_compilationTree->clear();
        m_inspectorDocument->setText(QStringLiteral("No document"));
        m_inspectorContext->clear();
        updateDocumentTabs();
        return;
    }
    m_parameterInspector->setModel(document->parameters());
    const bool boundScene = document == m_renderToySession->sceneDocument();
    const bool boundPost = document == m_renderToySession->postDocument();
    const QString binding = boundScene && boundPost ? QStringLiteral("Bound as Scene and Post")
        : boundScene                                ? QStringLiteral("Bound as Scene")
        : boundPost                                 ? QStringLiteral("Bound as Post")
                                                    : QStringLiteral("Not bound to Render Toy");
    m_inspectorDocument->setText(m_workspace->displayName(document));
    m_inspectorContext->setText(binding);
    m_diagnostics->setPlainText(m_workspace->focusedDocument()->diagnostics());
    refreshDependencyInspector();
    refreshSemanticInspector();
    if (m_lsp) {
        const QString uri = documentUri(m_workspace->focusedDocument());
        m_editor->setLanguageClient(m_lsp, uri);
        m_editor->setDiagnostics(m_diagnosticsByUri.value(uri));
    }
    recountDiagnostics();
    m_lastCompileOk = m_workspace->focusedDocument()->compileSucceeded();
    setCompileState(m_lastCompileOk ? (m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok)
                                    : CompileState::Error);
    m_bindScene->setText(
        boundScene ? QStringLiteral("Bound as Scene") : QStringLiteral("Use as Scene"));
    m_bindPost->setText(
        boundPost ? QStringLiteral("Bound as Post") : QStringLiteral("Use as Post"));
    m_bindScene->setEnabled(!boundScene);
    m_bindPost->setEnabled(!boundPost);
    auto markViewportActive = [](QWidget* viewport, bool active) {
        viewport->setProperty("documentFocus", active);
        viewport->style()->unpolish(viewport);
        viewport->style()->polish(viewport);
    };
    markViewportActive(m_sceneViewport, boundScene);
    markViewportActive(m_viewport, boundPost);
    updateDocumentTabs();
}

void WorkbenchWindow::updateDocumentTabs()
{
    if (!m_workspace || !m_sceneBinding || !m_postBinding)
        return;
    if (m_bindingSummary) {
        const QString scene = m_workspace->displayName(m_renderToySession->sceneDocument());
        const QString post = m_workspace->displayName(m_renderToySession->postDocument());
        m_bindingSummary->setText(QStringLiteral("Scene: %1    ·    Post: %2").arg(scene, post));
    }
    QSignalBlocker sceneBlock(m_sceneBinding);
    QSignalBlocker postBlock(m_postBinding);
    m_sceneBinding->clear();
    m_postBinding->clear();
    int sceneIndex = -1;
    int postIndex = -1;
    for (int i = 0; i < m_workspace->documentCount(); ++i) {
        ShaderDocument* doc = m_workspace->documentAt(i);
        const QString name = m_workspace->displayName(doc);
        m_sceneBinding->addItem(name);
        m_postBinding->addItem(name);
        if (doc == m_renderToySession->sceneDocument())
            sceneIndex = i;
        if (doc == m_renderToySession->postDocument())
            postIndex = i;
    }
    m_sceneBinding->setCurrentIndex(sceneIndex);
    m_postBinding->setCurrentIndex(postIndex);
}

void WorkbenchWindow::loadSample(const QString& name, int target, const QByteArray& source)
{
    ShaderDocument* doc = m_workspace->openSource(
        QUrl(QStringLiteral("workbench:/samples/") + name), name, QString::fromUtf8(source));
    if (target == 0)
        m_renderToySession->bindScene(doc);
    else
        m_renderToySession->bindPost(doc);
    doc->compile();
    statusBar()->showMessage(QStringLiteral("Opened %1").arg(name), 1600);
}

QString WorkbenchWindow::documentUri(ShaderDocument* doc) const
{
    if (doc && !doc->fileUrl().isEmpty())
        return doc->fileUrl().toString();
    return QStringLiteral("workbench:/untitled.slang");
}

void WorkbenchWindow::recountDiagnostics()
{
    m_editorErrors = 0;
    m_editorWarnings = 0;
    if (!m_workspace->focusedDocument())
        return;
    for (const LspDiagnostic& d :
        m_diagnosticsByUri.value(documentUri(m_workspace->focusedDocument()))) {
        if (d.severity == 1)
            ++m_editorErrors;
        else if (d.severity == 2)
            ++m_editorWarnings;
    }
}

void WorkbenchWindow::setCompileState(CompileState state)
{
    m_compileState = state;
    updateCompileStatus();
}

void WorkbenchWindow::updateCompileStatus()
{
    if (!m_compileStatus)
        return;

    QString text;
    QString state; // drives the [state=...] stylesheet selector
    QString tip;
    switch (m_compileState) {
    case CompileState::Compiling:
        text = QStringLiteral("Compiling…");
        state = QStringLiteral("compiling");
        tip = QStringLiteral("Compiling the shader…");
        break;
    case CompileState::Dirty:
        text = QStringLiteral("● Modified");
        state = QStringLiteral("dirty");
        tip = QStringLiteral("Unsaved edits — compiling shortly. Click to compile now.");
        break;
    default: {
        const int ms
            = m_workspace->focusedDocument() ? m_workspace->focusedDocument()->lastCompileMs() : -1;
        if (!m_lastCompileOk || m_editorErrors > 0) {
            const int n = qMax(1, m_editorErrors);
            text = QStringLiteral("✕  %1 error%2")
                       .arg(n)
                       .arg(n == 1 ? QString() : QStringLiteral("s"));
            if (m_editorWarnings > 0)
                text += QStringLiteral(", %1 warning%2")
                            .arg(m_editorWarnings)
                            .arg(m_editorWarnings == 1 ? QString() : QStringLiteral("s"));
            state = QStringLiteral("error");
            tip = QStringLiteral("Compile failed. Click to jump to the first error.");
        } else if (m_editorWarnings > 0) {
            text = QStringLiteral("✓  Compiled · %1 warning%2")
                       .arg(m_editorWarnings)
                       .arg(m_editorWarnings == 1 ? QString() : QStringLiteral("s"));
            state = QStringLiteral("warn");
            tip = QStringLiteral("Compiled with warnings. Click to jump to the first one.");
        } else {
            text = ms >= 0 ? QStringLiteral("✓  Compiled · %1 ms").arg(ms)
                           : QStringLiteral("✓  Compiled");
            state = QStringLiteral("ok");
            tip = QStringLiteral("Shader is up to date. Click to recompile (Ctrl+Enter).");
        }
        break;
    }
    }

    m_compileStatus->setText(text);
    m_compileStatus->setToolTip(tip);
    if (m_compileStatus->property("state").toString() != state) {
        m_compileStatus->setProperty("state", state);
        m_compileStatus->style()->unpolish(m_compileStatus);
        m_compileStatus->style()->polish(m_compileStatus);
    }

    if (m_tabs && m_diagTabIndex >= 0) {
        m_tabs->setTabText(m_diagTabIndex,
            m_editorErrors > 0 ? QStringLiteral("Compilation (%1)").arg(m_editorErrors)
                               : QStringLiteral("Compilation"));
    }
}

void WorkbenchWindow::jumpToFirstError()
{
    if (!m_workspace->focusedDocument())
        return;
    const QList<LspDiagnostic> diags
        = m_diagnosticsByUri.value(documentUri(m_workspace->focusedDocument()));

    // Prefer the earliest error; if there are none, fall back to the earliest warning.
    auto earliest = [&diags](int severity) -> const LspDiagnostic* {
        const LspDiagnostic* best = nullptr;
        for (const LspDiagnostic& d : diags) {
            if (d.severity != severity)
                continue;
            if (!best || d.range.startLine < best->range.startLine
                || (d.range.startLine == best->range.startLine
                    && d.range.startChar < best->range.startChar))
                best = &d;
        }
        return best;
    };
    const LspDiagnostic* target = earliest(1);
    if (!target)
        target = earliest(2);

    if (m_tabs && m_diagTabIndex >= 0)
        m_tabs->setCurrentIndex(m_diagTabIndex);
    if (target)
        m_editor->goToPosition(target->range.startLine, target->range.startChar);
}

void WorkbenchWindow::setupLanguageServer()
{
    QString exe = qEnvironmentVariable("SLANGD_PATH");
    if (exe.isEmpty() || !QFileInfo::exists(exe))
        exe = QStandardPaths::findExecutable(QStringLiteral("slangd"));
    if (exe.isEmpty()) {
        QStringList candidates;
        const QString slangRoot = qEnvironmentVariable("SLANG_ROOT");
        if (!slangRoot.isEmpty())
            candidates << slangRoot + QStringLiteral("/bin/slangd.exe");
        candidates << QCoreApplication::applicationDirPath() + QStringLiteral("/slangd.exe");
        for (const QString& candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                exe = candidate;
                break;
            }
        }
    }
    if (exe.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("Slang language server (slangd) not found — IDE features disabled"),
            4000);
        return;
    }

    m_lsp = new LspClient(this);
    connect(m_lsp, &LspClient::diagnosticsReceived, this,
        [this](const QString& uri, const QList<LspDiagnostic>& diagnostics) {
            m_diagnosticsByUri.insert(uri, diagnostics);
            if (m_workspace->focusedDocument()
                && documentUri(m_workspace->focusedDocument()) == uri) {
                m_editor->setDiagnostics(diagnostics);
                recountDiagnostics();
                updateCompileStatus();
            }
        });
    connect(m_lsp, &LspClient::ready, this,
        [this] { statusBar()->showMessage(QStringLiteral("Slang language server ready"), 2000); });
    m_lsp->start(exe);
    m_lsp->openDocument(documentUri(m_sceneDocument), m_sceneDocument->source());
    m_lsp->openDocument(documentUri(m_document), m_document->source());
}

void WorkbenchWindow::reloadGeneratedTargets()
{
    if (!m_generatedTarget || !m_workspace->focusedDocument())
        return;
    const QString current = m_generatedTarget->currentText();
    const QStringList targets = m_workspace->focusedDocument()->generatedTargets();
    QSignalBlocker block(m_generatedTarget);
    m_generatedTarget->clear();
    m_generatedTarget->addItems(targets);
    const int idx = targets.indexOf(current);
    if (idx >= 0)
        m_generatedTarget->setCurrentIndex(idx);
    block.unblock();
    refreshGeneratedView();
}

void WorkbenchWindow::refreshDependencyInspector()
{
    m_dependencyTree->clear();
    m_dependencySource->clear();
    if (!m_workspace->focusedDocument())
        return;

    auto* graph = m_workspace->focusedDocument()->dependencyGraph();
    const NodeId sourceNode = graph->nodeId(QStringLiteral("source:user"));
    auto* shader = new QTreeWidgetItem(m_dependencyTree,
        { QStringLiteral("Shader"), m_workspace->displayName(m_workspace->focusedDocument()),
            graph->digestHex(sourceNode).left(12),
            graph->dirtyFlags(sourceNode) ? QStringLiteral("dirty") : QStringLiteral("clean") });
    shader->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(sourceNode));
    for (const SourceDependency& dependency :
        m_workspace->focusedDocument()->importedDependencies()) {
        const NodeId node = graph->nodeId(QStringLiteral("module:") + dependency.identity);
        auto* item = new QTreeWidgetItem(shader,
            { QStringLiteral("Import"), dependency.identity,
                QString::fromLatin1(dependency.digest.toHex().left(12)),
                graph->dirtyFlags(node) ? QStringLiteral("dirty") : QStringLiteral("clean") });
        item->setToolTip(1, dependency.path);
        item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(node));
    }

    // Internal products remain available through progressive disclosure, but the
    // default dependency UX is the authored shader/module stack above.
    auto* advanced = new QTreeWidgetItem(m_dependencyTree,
        { QStringLiteral("Advanced graph"), QStringLiteral("Compiler / renderer products"),
            QString(), QString() });
    auto kindName = [](NodeKind kind) {
        switch (kind) {
        case NodeKind::Source:
            return QStringLiteral("Source");
        case NodeKind::Module:
            return QStringLiteral("Import");
        case NodeKind::EntryPoint:
            return QStringLiteral("Entry point");
        case NodeKind::UiSchema:
            return QStringLiteral("UI schema");
        case NodeKind::ParameterLayout:
            return QStringLiteral("Layout");
        case NodeKind::ParameterValues:
            return QStringLiteral("Values");
        case NodeKind::Resource:
            return QStringLiteral("Resource");
        case NodeKind::BindingLayout:
            return QStringLiteral("Bindings");
        case NodeKind::RenderState:
            return QStringLiteral("Render state");
        case NodeKind::Pipeline:
            return QStringLiteral("Pipeline");
        }
        return QStringLiteral("Node");
    };
    std::function<void(NodeId, QTreeWidgetItem*)> addNode;
    addNode = [this, graph, &addNode, &kindName](NodeId node, QTreeWidgetItem* parent) {
        const QString key = graph->nodeKey(node);
        if (key.isEmpty())
            return;
        auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_dependencyTree);
        item->setText(0, kindName(graph->nodeKind(node)));
        item->setText(1, key.startsWith(QStringLiteral("module:")) ? key.mid(7) : key);
        item->setText(2, graph->digestHex(node).left(12));
        item->setText(
            3, graph->dirtyFlags(node) ? QStringLiteral("dirty") : QStringLiteral("clean"));
        item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(node));
        for (const QString& dependencyKey : graph->dependencies(node))
            addNode(graph->nodeId(dependencyKey), item);
    };
    addNode(graph->nodeId(QStringLiteral("rhi:pipeline")), advanced);
    shader->setExpanded(true);
    advanced->setExpanded(false);
}

void WorkbenchWindow::refreshGeneratedView()
{
    if (!m_generatedView)
        return;
    const QString target = m_generatedTarget ? m_generatedTarget->currentText() : QString();
    m_generatedView->setPlainText(m_workspace->focusedDocument()
            ? m_workspace->focusedDocument()->generatedCode(target)
            : QString());
}

void WorkbenchWindow::refreshSemanticInspector()
{
    m_resourceTree->clear();
    m_compilationTree->clear();
    if (!m_workspace->focusedDocument())
        return;

    for (const ResourceDescriptor& resource : m_workspace->focusedDocument()->resources()) {
        new QTreeWidgetItem(m_resourceTree,
            { resource.name, resource.kind, QString::number(resource.binding),
                QString::number(resource.space) });
    }
    if (m_workspace->focusedDocument()->resources().isEmpty())
        new QTreeWidgetItem(m_resourceTree,
            { QStringLiteral("No reflected resources"), QString(), QString(), QString() });

    new QTreeWidgetItem(m_compilationTree,
        { QStringLiteral("Status"),
            m_workspace->focusedDocument()->compileSucceeded()
                ? QStringLiteral("Compiled")
                : QStringLiteral("Failed / not compiled") });
    new QTreeWidgetItem(m_compilationTree,
        { QStringLiteral("Compile time"),
            m_workspace->focusedDocument()->lastCompileMs() >= 0
                ? QStringLiteral("%1 ms").arg(m_workspace->focusedDocument()->lastCompileMs())
                : QStringLiteral("—") });
    auto* entryPoints
        = new QTreeWidgetItem(m_compilationTree, { QStringLiteral("Entry points"), QString() });
    new QTreeWidgetItem(entryPoints, { QStringLiteral("Vertex"), QStringLiteral("vsMain") });
    new QTreeWidgetItem(entryPoints, { QStringLiteral("Fragment"), QStringLiteral("psMain") });
    auto* targets
        = new QTreeWidgetItem(m_compilationTree, { QStringLiteral("Targets"), QString() });
    for (const QString& target : m_workspace->focusedDocument()->generatedTargets())
        new QTreeWidgetItem(targets, { target, QStringLiteral("Generated") });
    new QTreeWidgetItem(m_compilationTree,
        { QStringLiteral("Runtime"),
            QStringLiteral("%1 / Qt %2 / QRhi %3")
                .arg(QSysInfo::prettyProductName(), QString::fromLatin1(qVersion()),
                    m_sceneViewport->backendName()) });
    m_compilationTree->expandAll();
}

void WorkbenchWindow::applyTheme()
{
    ui::WorkbenchTheme::apply(*this);
}

void WorkbenchWindow::mirrorParameter(
    ShaderDocument* target, const QString& name, const QVariant& value)
{
    if (m_syncing || !target || !core::ViewportCameraBinding::isCameraParameter(name))
        return;
    m_syncing = true;
    target->parameters()->setValue(name, value);
    m_syncing = false;
}

void WorkbenchWindow::openShader(const QString& path)
{
    if (!m_workspace->openFile(path))
        statusBar()->showMessage(QStringLiteral("Could not open %1").arg(path), 2200);
}

} // namespace miskeyed::workbench::slang_rhi
