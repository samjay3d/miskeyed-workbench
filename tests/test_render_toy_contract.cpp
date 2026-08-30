#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <QCoreApplication>
#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QByteArray prelude = "import miskeyed.time;\n";
    prelude += workbenchModuleSource(QStringLiteral("ui"));
    prelude += "\n#define WORKBENCH_RENDER_TOY_POST_PROCESS\n";
    prelude += workbenchModuleSource(QStringLiteral("render-toy"));

    ShaderDocument document;
    document.setSystemPrelude(QString::fromUtf8(prelude));
    document.setSource(QStringLiteral(R"SLANG(
struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };
[shader("vertex")]
VSOut vsMain(uint id : SV_VertexID) { VSOut output; output.position = 0; output.uv = 0; return output; }
[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0 { return sampleScene(input.uv); }
)SLANG"));
    document.compile();

    assert(document.diagnostics().isEmpty());
    assert(document.fragmentShader().isValid());
    const auto dependencies = document.importedDependencies();
    assert(dependencies.size() == 1);
    assert(dependencies.front().identity == QStringLiteral("miskeyed.time"));
    const auto* graph = document.dependencyGraph();
    assert(graph->contains(QStringLiteral("module:miskeyed.time")));
    assert(graph->dependencies(graph->nodeId(QStringLiteral("shader:entrypoints")))
            .contains(QStringLiteral("module:miskeyed.time")));
    return 0;
}
