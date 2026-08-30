#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <miskeyed/workbench/modes/shader_toy/ShaderToySession.h>
#include <miskeyed/workbench/core/DependencyGraph.h>
#include <miskeyed/workbench/core/TimeContext.h>
#include <miskeyed/workbench/core/TimeTransport.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <miskeyed/workbench/ui/WorkbenchToolFactory.h>
#include <QCoreApplication>
#include <QFile>
#include <cassert>
#include <type_traits>

using namespace miskeyed::workbench::slang_rhi;

static_assert(std::is_abstract_v<WorkbenchToolUiSession>);

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ShaderWorkspace workspace;
    ShaderToySession shaderToy;
    RenderToySession renderToy;
    renderToy.setEvaluationContext(workspace.timeContext(), workspace.timeTransport());
    auto* scene = workspace.openSource(QUrl(QStringLiteral("workbench://samples/scene")),
        QStringLiteral("scene.slang"), QStringLiteral("// scene"));
    auto* post = workspace.openSource(QUrl(QStringLiteral("workbench://samples/post")),
        QStringLiteral("post.slang"), QStringLiteral("// post"));
    auto* generated = workspace.openSource(QUrl(QStringLiteral("generated://materialx/clouds")),
        QStringLiteral("clouds.slang"), QStringLiteral("// generated"));

    renderToy.bindScene(scene);
    renderToy.bindPost(post);
    shaderToy.bindShader(generated);
    assert(shaderToy.shaderDocument() == generated);
    assert(renderToy.sceneDocument() == scene);
    assert(renderToy.postDocument() == post);
    assert(workspace.documentCount() == 3);

    workspace.session(generated)->cursorPosition = 73;
    workspace.session(generated)->verticalScroll = 240;
    workspace.timeTransport()->seek(core::TimeValue(49.0, 24.0));

    generated->setSource(QStringLiteral("// edited once"));
    assert(generated->dirty());
    workspace.focusDocument(scene); // Represents switching back to the Render Toy context.
    assert(renderToy.sceneDocument() == scene);
    assert(renderToy.postDocument() == post);
    workspace.focusDocument(generated); // Represents returning to Shader Toy.
    assert(shaderToy.shaderDocument() == generated);
    assert(workspace.session(generated)->cursorPosition == 73);
    assert(workspace.session(generated)->verticalScroll == 240);
    assert(workspace.timeContext()->timeValue() == 49.0);
    assert(workspace.documentCount() == 3);

    Q_INIT_RESOURCE(shader_toy_samples);
    QFile sample(QStringLiteral(":/miskeyed/workbench/shader_toy/default.slang"));
    assert(sample.open(QIODevice::ReadOnly));
    ShaderDocument fullscreen;
    fullscreen.setSystemPrelude(QString());
    fullscreen.setSource(QString::fromUtf8(sample.readAll()));
    fullscreen.compile();
    assert(fullscreen.diagnostics().isEmpty());
    assert(fullscreen.vertexShader().isValid());
    assert(fullscreen.fragmentShader().isValid());
    assert(fullscreen.dependencyGraph()->contains(QStringLiteral("module:miskeyed.time")));
    assert(fullscreen.dependencyGraph()->contains(QStringLiteral("module:miskeyed.shader_toy")));
    return 0;
}
