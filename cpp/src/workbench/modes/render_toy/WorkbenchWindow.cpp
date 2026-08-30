#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <miskeyed/workbench/modes/render_toy/WorkbenchWindow.h>
#include <miskeyed/workbench/ui/ParameterInspector.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/editor/CodeEditor.h>
#include <miskeyed/workbench/editor/ShaderHighlighter.h>
#include <miskeyed/workbench/editor/LspClient.h>
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
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
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

void WorkbenchWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Workbench"));
    resize(1600, 950);
    m_timeContext = new core::TimeContext(this);
    m_timeTransport = new core::TimeTransport(this);
    m_timeTransport->setContext(m_timeContext);
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setTimerType(Qt::PreciseTimer);
    m_workspace = new ShaderWorkspace(this);
    m_sceneDocument
        = m_workspace->openSource(QUrl(QStringLiteral("workbench:/samples/scene_default.slang")),
            QStringLiteral("scene_default.slang"), ShaderRole::Scene,
            QString::fromUtf8(sceneShaderSource()));
    m_document
        = m_workspace->openSource(QUrl(QStringLiteral("workbench:/samples/post_default.slang")),
            QStringLiteral("post_default.slang"), ShaderRole::Post,
            QString::fromUtf8(postShaderSource()));

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
    m_editor = new CodeEditor(this);
    m_editor->setTabStopDistance(32);
    m_editor->setFont(monospaceFont());
    new ShaderHighlighter(m_editor->document());

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

    auto labelled = [this](const QString& title, QWidget* w) {
        auto* box = new QWidget(this);
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(0);
        auto* lbl = new QLabel(title, box);
        lbl->setObjectName(QStringLiteral("PanelHeader"));
        v->addWidget(lbl);
        v->addWidget(w, 1);
        return box;
    };

    auto* views = new QSplitter(Qt::Horizontal, this);
    views->addWidget(labelled(QStringLiteral("1 · Scene  —  renders into the G-buffer   (drag: "
                                             "orbit · middle: pan · right/wheel: zoom)"),
        m_sceneViewport));
    views->addWidget(labelled(
        QStringLiteral("2 · Post-Process  —  samples the scene texture and grades it on top"),
        m_viewport));
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

    auto* resources = new QPlainTextEdit(inspector);
    resources->setReadOnly(true);
    resources->setPlainText(QStringLiteral(
        "No reflected resources in this document.\n\nTextures, samplers, and buffers will appear "
        "here when resource reflection is exposed by the document model."));

    auto* tabs = new QTabWidget(inspector);
    tabs->setObjectName(QStringLiteral("ActiveDocumentInspector"));
    tabs->addTab(m_parameterInspector, QStringLiteral("Parameters"));
    tabs->addTab(resources, QStringLiteral("Resources"));
    // Reflection belongs in the inspector. Source and generated text are editor views,
    // not permanent inspector panels.
    tabs->addTab(dependencyPanel, QStringLiteral("Dependencies"));
    m_diagTabIndex = tabs->addTab(m_diagnostics, QStringLiteral("Compilation"));
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
    auto* frameSpin = new QSpinBox(timeline);
    auto* startSpin = new QSpinBox(timeline);
    auto* endSpin = new QSpinBox(timeline);
    auto* fpsSpin = new QDoubleSpinBox(timeline);
    auto* timeLabel = new QLabel(timeline);
    auto* frameSlider = new QSlider(Qt::Horizontal, timeline);
    for (QSpinBox* spin : { frameSpin, startSpin, endSpin })
        spin->setRange(-1000000, 1000000);
    auto syncTransportRange = [this, frameSpin, frameSlider, startSpin, endSpin, fpsSpin] {
        const int start = qRound(m_timeTransport->startValue());
        const int end = qRound(m_timeTransport->endValue());
        frameSpin->setRange(start, end);
        frameSlider->setRange(start, end);
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
    connect(frameSpin, &QSpinBox::valueChanged, this, [this](int frame) {
        m_timeTransport->seek(core::TimeValue(double(frame), m_timeTransport->rate()));
    });
    connect(frameSlider, &QSlider::valueChanged, this, [this](int frame) {
        m_timeTransport->seek(core::TimeValue(double(frame), m_timeTransport->rate()));
    });
    connect(fpsSpin, &QDoubleSpinBox::valueChanged, m_timeTransport, &core::TimeTransport::setRate);
    auto setRange = [this, startSpin, endSpin] {
        m_timeTransport->setPlaybackRange(
            core::TimeValue(double(startSpin->value()), m_timeTransport->rate()),
            core::TimeValue(double(endSpin->value()), m_timeTransport->rate()));
    };
    connect(startSpin, &QSpinBox::valueChanged, this, setRange);
    connect(endSpin, &QSpinBox::valueChanged, this, setRange);
    connect(m_timeTransport, &core::TimeTransport::playingChanged, this,
        [this, playPause](bool playing) {
            playPause->setText(playing ? QStringLiteral("❚❚") : QStringLiteral("▶"));
            if (playing)
                m_playbackTimer->start(qMax(1, qRound(1000.0 / m_timeTransport->rate())));
            else
                m_playbackTimer->stop();
        });
    connect(m_playbackTimer, &QTimer::timeout, this, [this] { m_timeTransport->step(1.0); });
    connect(m_timeTransport, &core::TimeTransport::playbackChanged, this, syncTransportRange);
    connect(m_timeContext, &core::TimeContext::changed, this,
        [this, frameSpin, frameSlider, timeLabel] {
            QSignalBlocker frameBlock(frameSpin);
            QSignalBlocker sliderBlock(frameSlider);
            frameSpin->setValue(qRound(m_timeContext->timeValue()));
            frameSlider->setValue(qRound(m_timeContext->timeValue()));
            timeLabel->setText(
                QStringLiteral("Time %1 s").arg(m_timeContext->timeSeconds(), 0, 'f', 3));
            if (m_timeTransport->playing())
                m_playbackTimer->setInterval(qMax(1, qRound(1000.0 / m_timeTransport->rate())));
        });
    timeLabel->setText(QStringLiteral("Time 0.000 s"));
    auto* editorBar = new QWidget(editorBox);
    auto* eh = new QHBoxLayout(editorBar);
    eh->setContentsMargins(0, 0, 0, 0);
    m_documentTabs = new QTabBar(editorBar);
    m_documentTabs->setExpanding(false);
    m_documentTabs->setDocumentMode(true);
    m_documentTabs->setShape(QTabBar::RoundedNorth);
    m_documentTabs->setToolTip(QStringLiteral(
        "Open shader documents. [Scene] and [Post] mark the pair bound to Render Toy."));
    eh->addWidget(m_documentTabs, 1);

    // Document tabs occupy their own row. View and Render Toy actions belong to the
    // selected document below them instead of competing with filenames for space.
    auto* documentBar = new QWidget(editorBox);
    documentBar->setObjectName(QStringLiteral("DocumentViewBar"));
    auto* dh = new QHBoxLayout(documentBar);
    dh->setContentsMargins(8, 3, 8, 3);
    dh->addWidget(new QLabel(QStringLiteral("View"), documentBar));
    for (const QString& label :
        { QStringLiteral("Source"), QStringLiteral("Generated"), QStringLiteral("Compare") }) {
        auto* button = new QPushButton(label, documentBar);
        button->setCheckable(true);
        m_viewButtons.push_back(button);
        dh->addWidget(button);
    }
    dh->addSpacing(10);
    dh->addWidget(new QLabel(QStringLiteral("Render Toy role"), documentBar));
    auto* bindDocument = new QPushButton(QStringLiteral("Use focused document"), documentBar);
    m_bindDocument = bindDocument;
    bindDocument->setToolTip(
        QStringLiteral("Bind this document to its Scene or Post role without closing other tabs."));
    dh->addWidget(bindDocument);

    // Persistent, color-coded compile-status pill. Sits right next to the editor so
    // feedback is where the user is looking (Fitts's Law) and always visible (Doherty
    // Threshold / Visibility of System Status). Clicking it jumps to the first error,
    // or recompiles when the shader is already clean.
    m_compileStatus = new QPushButton(documentBar);
    m_compileStatus->setObjectName(QStringLiteral("CompileStatus"));
    m_compileStatus->setCursor(Qt::PointingHandCursor);
    m_compileStatus->setFocusPolicy(Qt::NoFocus);
    dh->addSpacing(6);
    dh->addWidget(m_compileStatus);

    auto* navHint = new QLabel(
        QStringLiteral("Camera: drag = orbit · middle = pan · right/wheel = zoom  (Houdini)"),
        documentBar);
    navHint->setObjectName(QStringLiteral("HintLabel"));
    dh->addStretch(1);
    dh->addWidget(navHint);

    // Left: the editable Slang source. Right: the compiled output for a chosen backend.
    auto* sourceSide = new QWidget(editorBox);
    m_sourceSide = sourceSide;
    auto* sv = new QVBoxLayout(sourceSide);
    sv->setContentsMargins(0, 0, 0, 0);
    sv->setSpacing(0);
    auto* sourceHeader = new QLabel(QStringLiteral("Slang source"), sourceSide);
    sourceHeader->setObjectName(QStringLiteral("PanelHeader"));
    sv->addWidget(sourceHeader);
    sv->addWidget(m_editor, 1);

    m_generatedView = new CodeEditor(editorBox);
    m_generatedView->setReadOnly(true);
    m_generatedView->setFont(monospaceFont());
    m_generatedView->setPlaceholderText(
        QStringLiteral("Compile the shader to see generated code."));
    new ShaderHighlighter(m_generatedView->document());

    auto* genSide = new QWidget(editorBox);
    m_generatedSide = genSide;
    auto* gv = new QVBoxLayout(genSide);
    gv->setContentsMargins(0, 0, 0, 0);
    gv->setSpacing(0);
    auto* genBar = new QWidget(genSide);
    auto* gh = new QHBoxLayout(genBar);
    gh->setContentsMargins(8, 3, 8, 3);
    auto* genTitle = new QLabel(QStringLiteral("Compiled output"), genBar);
    genTitle->setObjectName(QStringLiteral("PanelHeaderInline"));
    gh->addWidget(genTitle);
    m_generatedTarget = new QComboBox(genBar);
    m_generatedTarget->setToolTip(QStringLiteral(
        "Backend to disassemble the current shader to: HLSL, GLSL, SPIR-V or Metal.\n"
        "Slang cross-compiles your Slang source to each of these."));
    gh->addWidget(m_generatedTarget);
    gh->addStretch(1);
    auto* copyBtn = new QPushButton(QStringLiteral("Copy"), genBar);
    copyBtn->setToolTip(QStringLiteral("Copy the generated code to the clipboard."));
    gh->addWidget(copyBtn);
    genBar->setObjectName(QStringLiteral("PanelHeader"));
    gv->addWidget(genBar);
    gv->addWidget(m_generatedView, 1);
    connect(copyBtn, &QPushButton::clicked, this, [this] {
        if (m_generatedView)
            QGuiApplication::clipboard()->setText(m_generatedView->toPlainText());
        statusBar()->showMessage(QStringLiteral("Copied generated code"), 1200);
    });

    auto* editorSplit = new QSplitter(Qt::Horizontal, editorBox);
    m_editorSplit = editorSplit;
    editorSplit->addWidget(sourceSide);
    editorSplit->addWidget(genSide);
    editorSplit->setStretchFactor(0, 3);
    editorSplit->setStretchFactor(1, 2);
    ev->addWidget(editorBar);
    ev->addWidget(documentBar);
    ev->addWidget(editorSplit, 1);
    // Transport is a persistent controller for the active Render Toy evaluation,
    // independent of whichever authoring document currently has focus.
    ev->addWidget(timeline);
    setEditorView(0);

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
        if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->save();
        }
    });
    connect(compile, &QAction::triggered, this, [this] {
        if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->compile();
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
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
        if (!m_editorDoc)
            return;
        if (m_editorDoc->live())
            m_editorDoc->setSource(m_editor->toPlainText());
        setCompileState(CompileState::Dirty);
    });

    connect(m_compileStatus, &QPushButton::clicked, this, [this] {
        if (m_editorErrors > 0 || (!m_lastCompileOk && m_compileState == CompileState::Error))
            jumpToFirstError();
        else if (m_editorDoc) {
            m_editorDoc->setSource(m_editor->toPlainText());
            m_editorDoc->compile();
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
    connect(m_workspace, &ShaderWorkspace::renderBindingsChanged, this,
        [this](ShaderDocument* scene, ShaderDocument* post) {
            m_sceneDocument = scene;
            m_document = post;
            m_sceneViewport->setDocument(scene);
            m_viewport->setScenePass(scene);
            m_viewport->setDocument(post);
            // Binding changes affect rendering only. The inspector continues to follow
            // workspace focus, even when another document is bound behind the scenes.
            if (m_editorDoc)
                setFocusedDocument(m_editorDoc);
            updateDocumentTabs();
        });

    connect(m_sceneViewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });
    connect(m_viewport, &SlangRhiWidget::gpuError, this,
        [this](const QString& e) { m_diagnostics->appendPlainText(e); });

    connect(m_documentTabs, &QTabBar::currentChanged, this, [this](int index) {
        if (ShaderDocument* doc = m_workspace->documentAt(index))
            m_workspace->focusDocument(doc);
    });
    for (int i = 0; i < m_viewButtons.size(); ++i)
        connect(m_viewButtons.at(i), &QPushButton::clicked, this, [this, i] { setEditorView(i); });
    connect(m_bindDocument, &QPushButton::clicked, this, [this] {
        if (m_workspace->role(m_editorDoc) == ShaderRole::Scene)
            m_workspace->bindScene(m_editorDoc);
        else if (m_workspace->role(m_editorDoc) == ShaderRole::Post)
            m_workspace->bindPost(m_editorDoc);
    });
    connect(m_generatedTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this] { refreshGeneratedView(); });
    connect(
        m_dependencyTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
            if (!item || !m_editorDoc) {
                m_dependencySource->clear();
                return;
            }
            const NodeId node = item->data(0, Qt::UserRole).toULongLong();
            m_dependencySource->setPlainText(
                QString::fromUtf8(m_editorDoc->dependencyGraph()->payload(node)));
        });
    connect(m_sceneViewport, &SlangRhiWidget::activated, this,
        [this] { m_workspace->focusDocument(m_workspace->activeSceneDocument()); });
    connect(m_viewport, &SlangRhiWidget::activated, this,
        [this] { m_workspace->focusDocument(m_workspace->activePostDocument()); });
}

void WorkbenchWindow::hookDocument(ShaderDocument* doc)
{
    connect(doc, &ShaderDocument::sourceChanged, this, [this, doc] {
        if (doc == m_editorDoc && m_editor->toPlainText() != doc->source()) {
            QSignalBlocker block(m_editor);
            m_editor->setPlainText(doc->source());
        }
    });
    connect(doc, &ShaderDocument::dirtyChanged, this, [this] { updateDocumentTabs(); });
    connect(doc, &ShaderDocument::diagnosticsChanged, this, [this, doc] {
        if (doc == m_editorDoc)
            m_diagnostics->setPlainText(doc->diagnostics());
    });
    connect(doc, &ShaderDocument::dependenciesChanged, this, [this, doc] {
        if (doc == m_editorDoc)
            refreshDependencyInspector();
    });
    connect(doc->dependencyGraph(), &DependencyGraph::graphChanged, this, [this, doc] {
        if (doc == m_editorDoc)
            refreshDependencyInspector();
    });
    connect(
        doc->dependencyGraph(), &DependencyGraph::nodeChanged, this, [this, doc](quint64, quint32) {
            if (doc == m_editorDoc)
                refreshDependencyInspector();
        });
    connect(doc, &ShaderDocument::compilingChanged, this, [this, doc] {
        if (doc == m_editorDoc && doc->compiling()) {
            setCompileState(CompileState::Compiling);
            m_compileStatus->repaint();
        }
    });
    connect(doc, &ShaderDocument::compiled, this, [this, doc] {
        if (doc != m_editorDoc)
            return;
        m_lastCompileOk = true;
        recountDiagnostics();
        setCompileState(m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok);
        reloadGeneratedTargets();
    });
    connect(doc, &ShaderDocument::compileFailed, this, [this, doc](const QString&) {
        if (doc != m_editorDoc)
            return;
        m_lastCompileOk = false;
        recountDiagnostics();
        setCompileState(CompileState::Error);
    });
    connect(doc->parameters(), &ShaderParameterModel::parameterChanged, this,
        [this, doc](const QString& name, const QVariant& value, int, int) {
            if (doc == m_workspace->activeSceneDocument())
                mirrorParameter(m_workspace->activePostDocument(), name, value);
            else if (doc == m_workspace->activePostDocument())
                mirrorParameter(m_workspace->activeSceneDocument(), name, value);
        });
}

void WorkbenchWindow::setFocusedDocument(ShaderDocument* document)
{
    if (!document)
        return;
    m_editorDoc = document;
    m_parameterInspector->setModel(document->parameters());
    const ShaderRole role = m_workspace->role(document);
    const bool boundScene = document == m_workspace->activeSceneDocument();
    const bool boundPost = document == m_workspace->activePostDocument();
    const QString roleName = role == ShaderRole::Scene ? QStringLiteral("Scene document")
        : role == ShaderRole::Post                     ? QStringLiteral("Post document")
                                                       : QStringLiteral("Generic document");
    const QString binding = boundScene ? QStringLiteral(" · bound as Scene")
        : boundPost                    ? QStringLiteral(" · bound as Post")
                                       : QStringLiteral(" · not bound");
    m_inspectorDocument->setText(m_workspace->displayName(document));
    m_inspectorContext->setText(roleName + binding);
    m_editor->blockSignals(true);
    m_editor->setPlainText(m_editorDoc->source());
    m_editor->blockSignals(false);
    m_diagnostics->setPlainText(m_editorDoc->diagnostics());
    refreshDependencyInspector();
    if (m_lsp) {
        const QString uri = documentUri(m_editorDoc);
        m_editor->setLanguageClient(m_lsp, uri);
        m_editor->setDiagnostics(m_diagnosticsByUri.value(uri));
    }
    recountDiagnostics();
    m_lastCompileOk = m_editorDoc->compileSucceeded();
    setCompileState(m_lastCompileOk ? (m_editorWarnings > 0 ? CompileState::Warn : CompileState::Ok)
                                    : CompileState::Error);
    reloadGeneratedTargets();
    if (role == ShaderRole::Generic) {
        m_bindDocument->setText(QStringLiteral("Generic document"));
        m_bindDocument->setEnabled(false);
    } else if (boundScene || boundPost) {
        m_bindDocument->setText(
            boundScene ? QStringLiteral("Bound as Scene") : QStringLiteral("Bound as Post"));
        m_bindDocument->setEnabled(false);
    } else {
        m_bindDocument->setText(role == ShaderRole::Scene ? QStringLiteral("Use as Scene")
                                                          : QStringLiteral("Use as Post"));
        m_bindDocument->setEnabled(true);
    }
    auto markViewportActive = [](QWidget* viewport, bool active) {
        viewport->setProperty("documentFocus", active);
        viewport->style()->unpolish(viewport);
        viewport->style()->polish(viewport);
    };
    markViewportActive(m_sceneViewport, boundScene);
    markViewportActive(m_viewport, boundPost);
    updateDocumentTabs();
}

void WorkbenchWindow::setEditorView(int mode)
{
    mode = qBound(0, mode, 2);
    m_sourceSide->setVisible(mode != 1);
    m_generatedSide->setVisible(mode != 0);
    for (int i = 0; i < m_viewButtons.size(); ++i)
        m_viewButtons.at(i)->setChecked(i == mode);
}

void WorkbenchWindow::updateDocumentTabs()
{
    if (!m_documentTabs)
        return;
    QSignalBlocker blocker(m_documentTabs);
    while (m_documentTabs->count() < m_workspace->documentCount())
        m_documentTabs->addTab(QString());
    for (int i = 0; i < m_workspace->documentCount(); ++i) {
        ShaderDocument* doc = m_workspace->documentAt(i);
        QString suffix;
        if (doc == m_workspace->activeSceneDocument())
            suffix = QStringLiteral("  [Scene]");
        if (doc == m_workspace->activePostDocument())
            suffix = QStringLiteral("  [Post]");
        m_documentTabs->setTabText(i,
            m_workspace->displayName(doc) + (doc->dirty() ? QStringLiteral(" •") : QString())
                + suffix);
        if (doc == m_editorDoc)
            m_documentTabs->setCurrentIndex(i);
    }
    if (m_bindingSummary) {
        const QString scene = m_workspace->displayName(m_workspace->activeSceneDocument());
        const QString post = m_workspace->displayName(m_workspace->activePostDocument());
        m_bindingSummary->setText(QStringLiteral("Scene: %1    ·    Post: %2").arg(scene, post));
    }
}

void WorkbenchWindow::loadSample(const QString& name, int target, const QByteArray& source)
{
    const ShaderRole role = target == 0 ? ShaderRole::Scene : ShaderRole::Post;
    ShaderDocument* doc = m_workspace->openSource(
        QUrl(QStringLiteral("workbench:/samples/") + name), name, role, QString::fromUtf8(source));
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
    if (!m_editorDoc)
        return;
    for (const LspDiagnostic& d : m_diagnosticsByUri.value(documentUri(m_editorDoc))) {
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
        const int ms = m_editorDoc ? m_editorDoc->lastCompileMs() : -1;
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
    if (!m_editorDoc)
        return;
    const QList<LspDiagnostic> diags = m_diagnosticsByUri.value(documentUri(m_editorDoc));

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
            if (m_editorDoc && documentUri(m_editorDoc) == uri) {
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
    if (!m_generatedTarget || !m_editorDoc)
        return;
    const QString current = m_generatedTarget->currentText();
    const QStringList targets = m_editorDoc->generatedTargets();
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
    if (!m_editorDoc)
        return;

    auto* graph = m_editorDoc->dependencyGraph();
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
    addNode(graph->nodeId(QStringLiteral("rhi:pipeline")), nullptr);
    m_dependencyTree->expandAll();
}

void WorkbenchWindow::refreshGeneratedView()
{
    if (!m_generatedView)
        return;
    const QString target = m_generatedTarget ? m_generatedTarget->currentText() : QString();
    m_generatedView->setPlainText(m_editorDoc ? m_editorDoc->generatedCode(target) : QString());
}

void WorkbenchWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #16171b; color: #c8ccd4; }
        QToolTip { background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47; padding: 4px; }
        QLabel#PanelHeader, QWidget#PanelHeader {
            background: #22242b; color: #e6e6e6; font-weight: 600;
            padding: 4px 10px; border-bottom: 1px solid #2f323b;
        }
        QLabel#PanelHeaderInline { color: #e6e6e6; font-weight: 600; padding-right: 6px; }
        QLabel#HintLabel { color: #7f8794; padding-left: 12px; }
        QWidget#InspectorPanel { border-left: 1px solid #2f323b; }
        QWidget#InspectorHeader { background: #1d1f25; border-bottom: 1px solid #2f323b; }
        QLabel#InspectorDocument { color: #e6e6e6; font-weight: 600; }
        QLabel#InspectorContext { color: #9aa0ac; }
        QLabel#BindingSummary { color: #aab0bc; padding-left: 8px; }
        #SceneViewport[documentFocus="true"], #PostViewport[documentFocus="true"] {
            border: 2px solid #7c5ce7;
        }
        QWidget#DocumentViewBar {
            background: #22242b; border-top: 1px solid #2f323b;
            border-bottom: 1px solid #2f323b;
        }
        QWidget#Timeline {
            background: #1d1f25; border-top: 1px solid #2f323b;
            border-bottom: 1px solid #2f323b;
        }
        QPlainTextEdit {
            background: #1b1c22; color: #c8ccd4; border: none;
            selection-background-color: #33467c; selection-color: #ffffff;
        }
        QToolBar { background: #1d1f25; border: none; spacing: 4px; padding: 4px; }
        QToolBar QToolButton {
            color: #d5d9e0; padding: 5px 12px; border-radius: 6px; background: transparent;
        }
        QToolBar QToolButton:hover { background: #2c2f38; }
        QToolBar QToolButton:pressed, QToolBar QToolButton:checked { background: #3a5fbf; color: #ffffff; }
        QToolBar::separator { background: #2f323b; width: 1px; margin: 4px 6px; }
        QPushButton {
            background: #2c2f38; color: #d5d9e0; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 12px;
        }
        QPushButton:hover { background: #363a45; border-color: #4a4e5a; }
        QPushButton:pressed { background: #3a5fbf; color: #ffffff; }
        QPushButton#CompileStatus {
            font-weight: 600; padding: 3px 12px; border-radius: 10px; border: 1px solid transparent;
        }
        QPushButton#CompileStatus[state="ok"]        { background: #17321f; color: #9ece6a; border-color: #2c5335; }
        QPushButton#CompileStatus[state="ok"]:hover   { border-color: #9ece6a; }
        QPushButton#CompileStatus[state="warn"]      { background: #33301b; color: #e0af68; border-color: #5a4d2e; }
        QPushButton#CompileStatus[state="warn"]:hover { border-color: #e0af68; }
        QPushButton#CompileStatus[state="error"]     { background: #35191f; color: #f7768e; border-color: #5a2b36; }
        QPushButton#CompileStatus[state="error"]:hover{ border-color: #f7768e; }
        QPushButton#CompileStatus[state="compiling"] { background: #1a2740; color: #7aa2f7; border-color: #2e4370; }
        QPushButton#CompileStatus[state="dirty"]     { background: #2a2c36; color: #c0caf5; border-color: #3a3d47; }
        QPushButton#CompileStatus[state="dirty"]:hover{ border-color: #7aa2f7; }
        QComboBox {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 26px 3px 10px; min-height: 20px;
        }
        QComboBox:hover { border-color: #4a4e5a; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            selection-background-color: #3a5fbf; selection-color: #ffffff; outline: none;
        }
        QTabWidget::pane { border: 1px solid #2f323b; background: #1b1c22; }
        QTabBar::tab {
            background: #1d1f25; color: #9aa0ac; padding: 6px 16px;
            border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px;
        }
        QTabBar::tab:selected { background: #22242b; color: #e6e6e6; border-bottom: 2px solid #7aa2f7; }
        QTabBar::tab:hover:!selected { color: #c8ccd4; }
        QSplitter::handle { background: #0f1013; }
        QSplitter::handle:horizontal { width: 3px; }
        QSplitter::handle:vertical { height: 3px; }
        QSplitter::handle:hover { background: #3a5fbf; }
        QStatusBar { background: #1d1f25; color: #9aa0ac; border-top: 1px solid #2f323b; }
        QScrollBar:vertical { background: #1b1c22; width: 12px; margin: 0; }
        QScrollBar::handle:vertical { background: #353842; min-height: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: #454956; }
        QScrollBar:horizontal { background: #1b1c22; height: 12px; margin: 0; }
        QScrollBar::handle:horizontal { background: #353842; min-width: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:horizontal:hover { background: #454956; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    )"));
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
