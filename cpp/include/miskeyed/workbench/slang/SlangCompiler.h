#pragma once

#include <miskeyed/workbench/Export.h>
#include <miskeyed/workbench/slang/ShaderParameterModel.h>
#include <QObject>
#include <QMap>
#include <rhi/qshader.h>
#include <memory>

namespace miskeyed::workbench::slang_rhi {

enum class ShaderStage : quint8 { Unknown, Vertex, Fragment, Compute };
[[nodiscard]] MISKEYED_WORKBENCH_SLANG_RHI_EXPORT QString shaderStageName(ShaderStage stage);

struct CompiledEntryPoint {
    QString name;
    ShaderStage stage = ShaderStage::Unknown;
    QString sourceIdentity;
    QShader shader;
    QByteArray identity;
    // Human-readable generated code per backend target ("HLSL", "GLSL", "SPIR-V",
    // "Metal") for the editor's "compiled output" viewer. Populated best-effort; a
    // target that is unavailable or fails simply does not appear.
    QMap<QString, QString> generated;
};

struct SourceDependency {
    QString identity;
    QString path;
    QByteArray source;
    QByteArray digest;
    QStringList imports;
};

struct ResourceDescriptor {
    QString name;
    QString kind;
    int binding = -1;
    int space = 0;
};

struct CompileResult {
    bool ok = false;
    QString diagnostics;
    QList<CompiledEntryPoint> entryPoints;
    QList<ParameterDescriptor> parameters;
    qsizetype parameterByteSize = 0;
    int parameterBinding = 0;
    QByteArray parameterLayoutDigest;
    QByteArray uiSchemaDigest;
    QList<SourceDependency> dependencies;
    QList<ResourceDescriptor> resources;
};

class SlangCompilerPrivate;

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT SlangCompiler final : public QObject {
    Q_OBJECT
public:
    explicit SlangCompiler(QObject* parent = nullptr);
    ~SlangCompiler() override;

    void setSearchPaths(QStringList paths);
    [[nodiscard]] QStringList searchPaths() const;

    // The system prelude is prepended to every compile so shaders can use Workbench
    // attributes without copying declarations. Keep this contract small: authored modules
    // belong in Slang import resolution. Diagnostics point at authored text through a
    // #line reset. Callers may replace it for a different platform contract.
    void setSystemPrelude(const QString& source);
    [[nodiscard]] QString systemPrelude() const;

    CompileResult compileProgram(
        const QString& source, const QString& virtualPath = QStringLiteral("user_shader.slang"));

private:
    std::unique_ptr<SlangCompilerPrivate> d;
};

} // namespace miskeyed::workbench::slang_rhi
