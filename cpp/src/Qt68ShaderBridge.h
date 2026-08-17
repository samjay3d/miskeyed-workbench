#pragma once

#include <rhi/qshader.h>
#include <slang.h>
#include <slang_qrhi/ShaderParameterModel.h>

namespace slang_qrhi::qt68 {

QShader buildQShader(
    QShader::Stage stage,
    const QString& entryPoint,
    const QByteArray& spirv,
    const QByteArray& hlsl,
    const QByteArray& msl,
    slang::ProgramLayout* layout,
    const QList<ParameterDescriptor>& parameters);

} // namespace slang_qrhi::qt68
