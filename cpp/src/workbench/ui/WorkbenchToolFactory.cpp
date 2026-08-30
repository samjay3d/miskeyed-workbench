#include <miskeyed/workbench/ui/WorkbenchToolFactory.h>
#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/modes/shader_toy/ShaderToySession.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace miskeyed::workbench::slang_rhi {
namespace {
    struct Panel {
        QWidget* surface;
        QComboBox* binding;
        QPushButton* bind;
    };

    Panel panel(QWidget* parent, const QString& name, const QString& action, QWidget* viewport)
    {
        auto* surface = new QWidget(parent);
        auto* column = new QVBoxLayout(surface);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(0);
        auto* header = new QWidget(surface);
        header->setObjectName(QStringLiteral("PanelHeader"));
        auto* row = new QHBoxLayout(header);
        row->setContentsMargins(8, 3, 8, 3);
        auto* label = new QLabel(name, header);
        label->setObjectName(QStringLiteral("PanelHeaderInline"));
        row->addWidget(label);
        auto* binding = new QComboBox(header);
        row->addWidget(binding, 1);
        row->addWidget(new QLabel(QStringLiteral("linked"), header));
        auto* bind = new QPushButton(action, header);
        row->addWidget(bind);
        column->addWidget(header);
        column->addWidget(viewport, 1);
        return { surface, binding, bind };
    }

    template <typename BoundDocument>
    void repopulate(QComboBox* combo, ShaderWorkspace* workspace, BoundDocument bound)
    {
        QSignalBlocker block(combo);
        combo->clear();
        for (int i = 0; i < workspace->documentCount(); ++i) {
            auto* document = workspace->documentAt(i);
            combo->addItem(workspace->displayName(document));
            if (document == bound)
                combo->setCurrentIndex(i);
        }
    }

    class RenderToyUiSession final : public WorkbenchToolUiSession {
    public:
        RenderToyUiSession(QWidget* parent, ShaderWorkspace* workspace, RenderToySession* session,
            QWidget* sceneViewport, QWidget* postViewport)
            : WorkbenchToolUiSession(parent)
            , m_workspace(workspace)
            , m_session(session)
        {
            const Panel scene = panel(parent, QStringLiteral("Scene"),
                QStringLiteral("Use focused as Scene"), sceneViewport);
            const Panel post = panel(parent, QStringLiteral("Post"),
                QStringLiteral("Use focused as Post"), postViewport);
            m_scene = scene.binding;
            m_post = post.binding;
            auto* split = new QSplitter(Qt::Horizontal, parent);
            split->setObjectName(QStringLiteral("RenderToySurface"));
            split->addWidget(scene.surface);
            split->addWidget(post.surface);
            m_surface = split;
            connect(scene.bind, &QPushButton::clicked, this,
                [this] { m_session->bindScene(m_workspace->focusedDocument()); });
            connect(post.bind, &QPushButton::clicked, this,
                [this] { m_session->bindPost(m_workspace->focusedDocument()); });
            connect(m_scene, QOverload<int>::of(&QComboBox::activated), this,
                [this](int i) { m_session->bindScene(m_workspace->documentAt(i)); });
            connect(m_post, QOverload<int>::of(&QComboBox::activated), this,
                [this](int i) { m_session->bindPost(m_workspace->documentAt(i)); });
            auto refresh = [this] {
                repopulate(m_scene, m_workspace, m_session->sceneDocument());
                repopulate(m_post, m_workspace, m_session->postDocument());
            };
            connect(session, &RenderToySession::bindingsChanged, this, refresh);
            connect(workspace, &ShaderWorkspace::documentAdded, this, refresh);
            connect(workspace, &ShaderWorkspace::documentClosed, this, refresh);
            connect(workspace, &ShaderWorkspace::documentOrderChanged, this, refresh);
            refresh();
        }
        QString toolId() const override { return QStringLiteral("render-toy"); }
        QString title() const override { return QStringLiteral("Render Toy"); }
        QWidget* surface() const override { return m_surface; }
        QString statusSummary() const override
        {
            return QStringLiteral("Scene: %1 · Post: %2")
                .arg(m_workspace->displayName(m_session->sceneDocument()),
                    m_workspace->displayName(m_session->postDocument()));
        }

    private:
        ShaderWorkspace* m_workspace;
        RenderToySession* m_session;
        QWidget* m_surface;
        QComboBox* m_scene;
        QComboBox* m_post;
    };

    class ShaderToyUiSession final : public WorkbenchToolUiSession {
    public:
        ShaderToyUiSession(QWidget* parent, ShaderWorkspace* workspace, ShaderToySession* session,
            QWidget* viewport)
            : WorkbenchToolUiSession(parent)
            , m_workspace(workspace)
            , m_session(session)
        {
            const Panel p = panel(parent, QStringLiteral("ShaderToy"),
                QStringLiteral("Use focused document"), viewport);
            m_surface = p.surface;
            m_surface->setObjectName(QStringLiteral("ShaderToySurface"));
            m_binding = p.binding;
            connect(p.bind, &QPushButton::clicked, this,
                [this] { m_session->bindShader(m_workspace->focusedDocument()); });
            connect(m_binding, QOverload<int>::of(&QComboBox::activated), this,
                [this](int i) { m_session->bindShader(m_workspace->documentAt(i)); });
            auto refresh
                = [this] { repopulate(m_binding, m_workspace, m_session->shaderDocument()); };
            connect(session, &ShaderToySession::bindingChanged, this, refresh);
            connect(workspace, &ShaderWorkspace::documentAdded, this, refresh);
            connect(workspace, &ShaderWorkspace::documentClosed, this, refresh);
            connect(workspace, &ShaderWorkspace::documentOrderChanged, this, refresh);
            refresh();
        }
        QString toolId() const override { return QStringLiteral("shader-toy"); }
        QString title() const override { return QStringLiteral("ShaderToy"); }
        QWidget* surface() const override { return m_surface; }
        QString statusSummary() const override
        {
            return QStringLiteral("Shader: %1")
                .arg(m_workspace->displayName(m_session->shaderDocument()));
        }

    private:
        ShaderWorkspace* m_workspace;
        ShaderToySession* m_session;
        QWidget* m_surface;
        QComboBox* m_binding;
    };
} // namespace

QList<WorkbenchToolUiSession*> createBuiltinToolUiSessions(QWidget* parent,
    SlangRhiWidget* sceneViewport, SlangRhiWidget* postViewport, SlangRhiWidget* shaderToyViewport,
    ShaderWorkspace* workspace, RenderToySession* renderToy, ShaderToySession* shaderToy)
{
    return { new RenderToyUiSession(parent, workspace, renderToy, sceneViewport, postViewport),
        new ShaderToyUiSession(parent, workspace, shaderToy, shaderToyViewport) };
}
} // namespace miskeyed::workbench::slang_rhi
