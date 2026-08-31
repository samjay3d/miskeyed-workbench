#pragma once

#include <rhi/qshader.h>
#include <slang.h>
#include <miskeyed/workbench/slang/ShaderParameterModel.h>

namespace miskeyed::workbench::slang_rhi::qt68 {

QShader buildQShader(QShader::Stage stage, const QString& entryPoint, const QByteArray& spirv,
    const QByteArray& hlsl, const QByteArray& msl, slang::ProgramLayout* layout,
    const QList<ParameterDescriptor>& parameters);

} // namespace miskeyed::workbench::slang_rhi::qt68
