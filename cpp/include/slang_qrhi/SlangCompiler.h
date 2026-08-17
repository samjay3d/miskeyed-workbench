#pragma once

#include "Export.h"
#include "ShaderParameterModel.h"
#include <QObject>
#include <QMap>
#include <rhi/qshader.h>
#include <memory>

namespace slang_qrhi {

struct CompiledStage {
    QShader shader;
    QByteArray entryPointHash;
    // Human-readable generated code per backend target ("HLSL", "GLSL", "SPIR-V",
    // "Metal") for the editor's "compiled output" viewer. Populated best-effort; a
    // target that is unavailable or fails simply does not appear.
    QMap<QString, QString> generated;
};

struct CompileResult {
    bool ok = false;
    QString diagnostics;
    CompiledStage vertex;
    CompiledStage fragment;
    QList<ParameterDescriptor> parameters;
    qsizetype parameterByteSize = 0;
    int parameterBinding = 0;
    QByteArray reflectionDigest;
};

class SlangCompilerPrivate;

class SLANG_QRHI_EXPORT SlangCompiler final : public QObject
{
    Q_OBJECT
public:
    explicit SlangCompiler(QObject* parent = nullptr);
    ~SlangCompiler() override;

    void setSearchPaths(QStringList paths);
    [[nodiscard]] QStringList searchPaths() const;

    CompileResult compileFullscreen(
        const QString& source,
        const QString& virtualPath = QStringLiteral("user_shader.slang"),
        const QString& vertexEntry = QStringLiteral("vsMain"),
        const QString& fragmentEntry = QStringLiteral("psMain"));

private:
    std::unique_ptr<SlangCompilerPrivate> d;
};

} // namespace slang_qrhi
