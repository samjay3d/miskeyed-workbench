#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/ShaderWorkspace.h>
#include <QCoreApplication>
#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ShaderWorkspace workspace;
    auto* first = workspace.openSource(QUrl(QStringLiteral("workbench:/scene_a.slang")),
        QStringLiteral("scene_a.slang"), ShaderRole::Scene, QStringLiteral("// A"));
    auto* second = workspace.openSource(QUrl(QStringLiteral("workbench:/scene_b.slang")),
        QStringLiteral("scene_b.slang"), ShaderRole::Scene, QStringLiteral("// B"));
    auto* post = workspace.openSource(QUrl(QStringLiteral("workbench:/post.slang")),
        QStringLiteral("post.slang"), ShaderRole::Post, QStringLiteral("// post"));

    assert(workspace.documentCount() == 3);
    assert(workspace.activeSceneDocument() == second);
    assert(workspace.activePostDocument() == post);
    workspace.bindScene(first);
    assert(workspace.activeSceneDocument() == first);
    assert(workspace.documentCount() == 3);
    workspace.focusDocument(post);
    assert(workspace.focusedDocument() == post);
    assert(!first->dirty());

    first->setSource(QStringLiteral("// edited"));
    assert(first->dirty());
    assert(!first->dependencyIdentity().isEmpty());
    return 0;
}
