#include <miskeyed/workbench/ui/WorkbenchToolFactory.h>
#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/modes/shader_toy/ShaderToySession.h>
#include <miskeyed/workbench/rendering/SlangRhiWidget.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
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

    class RenderToyContribution final : public WorkbenchToolContribution {
    public:
        RenderToyContribution(QWidget* parent, ShaderWorkspace* workspace,
            RenderToySession* session, QWidget* sceneViewport, QWidget* postViewport)
            : WorkbenchToolContribution(parent)
            , m_workspace(workspace)
            , m_session(session)
        {
            const Panel scene = panel(parent, QStringLiteral("Scene"),
                QStringLiteral("Use focused as Scene"), sceneViewport);
            const Panel post = panel(parent, QStringLiteral("Post"),
                QStringLiteral("Use focused as Post"), postViewport);
            m_scene = scene.binding;
            m_post = post.binding;
            scene.surface->setObjectName(QStringLiteral("RenderToySceneView"));
            post.surface->setObjectName(QStringLiteral("RenderToyPostView"));
            m_views = { scene.surface, post.surface };
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
        QObject* session() const override { return m_session; }
        QList<QWidget*> primaryViews() const override { return m_views; }
        QString statusSummary() const override
        {
            return QStringLiteral("Scene: %1 · Post: %2")
                .arg(m_workspace->displayName(m_session->sceneDocument()),
                    m_workspace->displayName(m_session->postDocument()));
        }

    private:
        ShaderWorkspace* m_workspace;
        RenderToySession* m_session;
        QList<QWidget*> m_views;
        QComboBox* m_scene;
        QComboBox* m_post;
    };

    class ShaderToyContribution final : public WorkbenchToolContribution {
    public:
        ShaderToyContribution(QWidget* parent, ShaderWorkspace* workspace,
            ShaderToySession* session, QWidget* viewport)
            : WorkbenchToolContribution(parent)
            , m_workspace(workspace)
            , m_session(session)
        {
            const Panel p = panel(parent, QStringLiteral("ShaderToy"),
                QStringLiteral("Use focused document"), viewport);
            p.surface->setObjectName(QStringLiteral("ShaderToyView"));
            m_views = { p.surface };
            m_binding = p.binding;
            connect(p.bind, &QPushButton::clicked, this,
                [this] { m_session->bindShader(m_workspace->focusedDocument()); });
            connect(m_binding, QOverload<int>::of(&QComboBox::activated), this, [this](int i) {
                m_session->bindShader(
                    m_binding->itemData(i, Qt::UserRole).value<ShaderDocument*>());
            });
            auto refresh = [this] {
                QSignalBlocker block(m_binding);
                m_binding->clear();
                for (int i = 0; i < m_workspace->documentCount(); ++i) {
                    auto* document = m_workspace->documentAt(i);
                    if (!m_session->canBindShader(document))
                        continue;
                    m_binding->addItem(
                        m_workspace->displayName(document), QVariant::fromValue(document));
                    if (document == m_session->shaderDocument())
                        m_binding->setCurrentIndex(m_binding->count() - 1);
                }
            };
            auto hookDocument = [this, refresh](ShaderDocument* document) {
                connect(document, &ShaderDocument::compiled, this, refresh);
                connect(document, &ShaderDocument::compileFailed, this, refresh);
            };
            connect(session, &ShaderToySession::bindingChanged, this, refresh);
            connect(workspace, &ShaderWorkspace::documentAdded, this,
                [hookDocument, refresh](ShaderDocument* document) {
                    hookDocument(document);
                    refresh();
                });
            connect(workspace, &ShaderWorkspace::documentClosed, this, refresh);
            connect(workspace, &ShaderWorkspace::documentOrderChanged, this, refresh);
            for (int i = 0; i < workspace->documentCount(); ++i)
                hookDocument(workspace->documentAt(i));
            refresh();
        }
        QString toolId() const override { return QStringLiteral("shader-toy"); }
        QString title() const override { return QStringLiteral("ShaderToy"); }
        QObject* session() const override { return m_session; }
        QList<QWidget*> primaryViews() const override { return m_views; }
        QString statusSummary() const override
        {
            return QStringLiteral("Shader: %1")
                .arg(m_workspace->displayName(m_session->shaderDocument()));
        }

    private:
        ShaderWorkspace* m_workspace;
        ShaderToySession* m_session;
        QList<QWidget*> m_views;
        QComboBox* m_binding;
    };
} // namespace

WorkbenchToolContribution* createRenderToyContribution(QWidget* parent, ShaderWorkspace* workspace,
    RenderToySession* session, SlangRhiWidget* sceneViewport, SlangRhiWidget* postViewport)
{
    return new RenderToyContribution(parent, workspace, session, sceneViewport, postViewport);
}

WorkbenchToolContribution* createShaderToyContribution(QWidget* parent, ShaderWorkspace* workspace,
    ShaderToySession* session, SlangRhiWidget* viewport)
{
    return new ShaderToyContribution(parent, workspace, session, viewport);
}
} // namespace miskeyed::workbench::slang_rhi
