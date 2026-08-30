#include <miskeyed/workbench/slang/ShaderDocument.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <cassert>

using namespace miskeyed::workbench::slang_rhi;

static void writeModule(const QString& path, const QByteArray& source)
{
    QFile file(path);
    assert(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    assert(file.write(source) == source.size());
    file.close();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir project;
    assert(project.isValid());
    const QString modulePath = project.filePath(QStringLiteral("animated.slang"));
    const QString shaderPath = project.filePath(QStringLiteral("scene.slang"));
    writeModule(modulePath, "float importedValue() { return 1.0; }\n");

    ShaderDocument document;
    document.setSystemPrelude(QString());
    document.setFileUrl(QUrl::fromLocalFile(shaderPath));
    document.setSource(QStringLiteral(R"SLANG(
import animated;
struct VSOut { float4 position : SV_Position; };
[shader("vertex")] VSOut vsMain(uint id : SV_VertexID) { VSOut o; o.position = 0; return o; }
[shader("fragment")] float4 psMain(VSOut input) : SV_Target0 { return importedValue(); }
)SLANG"));
    document.compile();
    assert(document.diagnostics().isEmpty());
    assert(document.importedDependencies().size() == 1);
    const QByteArray originalDigest = document.importedDependencies().front().digest;

    QEventLoop changed;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(3000);
    QObject::connect(&timeout, &QTimer::timeout, &changed, &QEventLoop::quit);
    QObject::connect(&document, &ShaderDocument::compiled, &changed, &QEventLoop::quit);
    timeout.start();
    writeModule(modulePath, "float importedValue() { return 2.0; }\n");
    changed.exec();

    assert(timeout.isActive());
    assert(document.diagnostics().isEmpty());
    assert(document.importedDependencies().front().digest != originalDigest);
    const NodeId moduleNode = document.dependencyGraph()->nodeId(
        QStringLiteral("module:") + document.importedDependencies().front().identity);
    assert(moduleNode != 0);
    assert(document.dependencyGraph()->payload(moduleNode).contains("return 2.0"));
    return 0;
}
