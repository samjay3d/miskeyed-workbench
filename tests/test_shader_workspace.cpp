#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <miskeyed/workbench/modes/render_toy/RenderToySession.h>
#include <QCoreApplication>
#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ShaderWorkspace workspace;
    RenderToySession renderToy;
    renderToy.setEvaluationContext(workspace.timeContext(), workspace.timeTransport());
    assert(renderToy.timeContext() == workspace.timeContext());
    auto* first = workspace.openSource(QUrl(QStringLiteral("workbench:/scene_a.slang")),
        QStringLiteral("scene_a.slang"), QStringLiteral("// A"));
    auto* second = workspace.openSource(QUrl(QStringLiteral("workbench:/scene_b.slang")),
        QStringLiteral("scene_b.slang"), QStringLiteral("// B"));
    auto* post = workspace.openSource(QUrl(QStringLiteral("workbench:/post.slang")),
        QStringLiteral("post.slang"), QStringLiteral("// post"));

    assert(workspace.documentCount() == 3);
    assert(workspace.focusedDocument() == post);
    assert(renderToy.sceneDocument() == nullptr);
    assert(renderToy.postDocument() == nullptr);
    renderToy.bindScene(first);
    renderToy.bindPost(post);
    assert(renderToy.sceneDocument() == first);
    assert(renderToy.postDocument() == post);
    workspace.focusDocument(first);
    auto* firstSession = workspace.session(first);
    firstSession->cursorPosition = 42;
    firstSession->verticalScroll = 120;
    firstSession->viewMode = 2;
    firstSession->generatedTarget = QStringLiteral("HLSL");
    workspace.focusDocument(second);
    assert(workspace.session(first)->cursorPosition == 42);
    assert(workspace.session(first)->verticalScroll == 120);
    assert(workspace.session(first)->viewMode == 2);
    assert(workspace.session(first)->generatedTarget == QStringLiteral("HLSL"));

    assert(workspace.moveDocument(1, 0));
    assert(workspace.documentAt(0) == second);
    assert(workspace.closeDocument(post));
    assert(workspace.documentCount() == 2);

    first->setSource(QStringLiteral("// edited"));
    assert(first->dirty());
    assert(!first->dependencyIdentity().isEmpty());
    return 0;
}
