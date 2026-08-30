#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <miskeyed/workbench/slang/WorkbenchModules.h>
#include <QCoreApplication>
#include <QFile>
#include <rhi/qshader.h>
#include <array>
#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const QString imports = QStringLiteral("import miskeyed.ui;\n"
                                           "import miskeyed.time;\n"
                                           "import miskeyed.render_toy;\n");

    ShaderDocument document;
    document.setSystemPrelude(QString());
    document.setSource(imports + QStringLiteral(R"SLANG(
struct VSOut { float4 position : SV_Position; float2 uv : TEXCOORD0; };
[shader("vertex")]
VSOut vsMain(uint id : SV_VertexID) { VSOut output; output.position = 0; output.uv = 0; return output; }
[shader("fragment")]
float4 psMain(VSOut input) : SV_Target0 { return sampleScene(input.uv); }
)SLANG"));
    document.compile();

    assert(document.diagnostics().isEmpty());
    assert(document.fragmentShader().isValid());
    const QShaderKey spirvKey(QShader::SpirvShader, QShaderVersion(100), QShader::StandardShader);
    assert(document.vertexShader().shader(spirvKey).entryPoint() == QByteArrayLiteral("main"));
    assert(document.fragmentShader().shader(spirvKey).entryPoint() == QByteArrayLiteral("main"));
    assert(document.generatedTargets().contains(QStringLiteral("Metal")));
    const auto dependencies = document.importedDependencies();
    assert(dependencies.size() == 3);
    QStringList identities;
    for (const SourceDependency& dependency : dependencies)
        identities.push_back(dependency.identity);
    assert(identities.contains(QStringLiteral("miskeyed.ui")));
    assert(identities.contains(QStringLiteral("miskeyed.time")));
    assert(identities.contains(QStringLiteral("miskeyed.render_toy")));
    QStringList resourceNames;
    for (const ResourceDescriptor& resource : document.resources())
        resourceNames.push_back(resource.name);
    assert(resourceNames.contains(QStringLiteral("sceneColor")));
    assert(resourceNames.contains(QStringLiteral("sceneSampler")));
    const auto* graph = document.dependencyGraph();
    assert(graph->contains(QStringLiteral("module:miskeyed.time")));
    assert(graph->dependencies(graph->nodeId(QStringLiteral("shader:entrypoints")))
            .contains(QStringLiteral("module:miskeyed.time")));

    Q_INIT_RESOURCE(render_toy_samples);
    constexpr std::array samplePaths {
        ":/miskeyed/workbench/render_toy/scene_default.slang",
        ":/miskeyed/workbench/render_toy/scene_clouds.slang",
        ":/miskeyed/workbench/render_toy/post_default.slang",
        ":/miskeyed/workbench/render_toy/post_bloom.slang",
        ":/miskeyed/workbench/render_toy/post_crt.slang",
    };
    for (const char* path : samplePaths) {
        QFile sample(QString::fromUtf8(path));
        assert(sample.open(QIODevice::ReadOnly));
        const QByteArray source = sample.readAll();
        assert(source.contains("import miskeyed.time;"));
        assert(source.contains("workbenchTime."));
        ShaderDocument sampleDocument;
        sampleDocument.setSystemPrelude(QString());
        sampleDocument.setSource(QString::fromUtf8(source));
        sampleDocument.compile();
        assert(sampleDocument.diagnostics().isEmpty());
        assert(sampleDocument.fragmentShader().isValid());
    }
    return 0;
}
