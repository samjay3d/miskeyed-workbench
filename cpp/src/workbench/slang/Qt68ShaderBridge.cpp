#include "Qt68ShaderBridge.h"
#include <private/qshaderdescription_p.h>
#include <rhi/qshader.h>

namespace miskeyed::workbench::slang_rhi::qt68 {
namespace {

    QShaderDescription::VariableType qtType(ParameterType type)
    {
        using V = QShaderDescription::VariableType;
        switch (type) {
        case ParameterType::Float:
            return V::Float;
        case ParameterType::Float2:
            return V::Vec2;
        case ParameterType::Float3:
            return V::Vec3;
        case ParameterType::Float4:
            return V::Vec4;
        case ParameterType::Int:
            return V::Int;
        case ParameterType::Int2:
            return V::Int2;
        case ParameterType::Int3:
            return V::Int3;
        case ParameterType::Int4:
            return V::Int4;
        case ParameterType::UInt:
            return V::Uint;
        case ParameterType::UInt2:
            return V::Uint2;
        case ParameterType::UInt3:
            return V::Uint3;
        case ParameterType::UInt4:
            return V::Uint4;
        case ParameterType::Bool:
            return V::Bool;
        default:
            return V::Unknown;
        }
    }

    QShaderDescription makeDescription(
        slang::ProgramLayout* layout, const QList<ParameterDescriptor>& parameters)
    {
        QShaderDescription desc;
        auto* p = QShaderDescriptionPrivate::get(&desc);
        if (!layout)
            return desc;

        QShaderDescription::UniformBlock block;
        block.blockName = QByteArrayLiteral("$Globals");
        block.structName = QByteArrayLiteral("$Globals");
        block.binding = int(layout->getGlobalConstantBufferBinding());
        block.descriptorSet = 0;
        block.size = int(layout->getGlobalConstantBufferSize());

        for (const auto& param : parameters) {
            QShaderDescription::BlockVariable v;
            v.name = param.name.toUtf8();
            v.type = qtType(param.type);
            v.offset = int(param.offset);
            v.size = int(param.size);
            block.members.append(v);
        }
        if (block.size > 0)
            p->uniformBlocks.append(block);
        return desc;
    }

    void addCode(QShader& shader, QShader::Source source, int version, const QByteArray& bytes,
        const QByteArray& entry)
    {
        if (bytes.isEmpty())
            return;
        QShaderKey key(source, QShaderVersion(version), QShader::StandardShader);
        QShaderCode code(bytes, entry);
        shader.setShader(key, code);
    }

} // namespace

QShader buildQShader(QShader::Stage stage, const QString& entryPoint, const QByteArray& spirv,
    const QByteArray& hlsl, const QByteArray& msl, slang::ProgramLayout* layout,
    const QList<ParameterDescriptor>& parameters)
{
    QShader shader;
    shader.setStage(stage);
    shader.setDescription(makeDescription(layout, parameters));
    const auto entry = entryPoint.toUtf8();
    // Slang's SPIR-V emitter uses the standardized exported name `main` even when the
    // authored entry point is called vsMain/psMain. QRhi passes this string to
    // vkCreateGraphicsPipelines, so using the authored name makes Vulkan reject an
    // otherwise valid module with "Entry point not found".
    addCode(shader, QShader::SpirvShader, 100, spirv, QByteArrayLiteral("main"));
    addCode(shader, QShader::HlslShader, 50, hlsl, entry);
    // Slang 2026.14 emits the standard [[vertex]] / [[fragment]] function attributes.
    // Apple accepts that spelling starting with MSL 2.3; labelling the source as 2.0
    // makes QRhi deliberately compile it with an older language mode and reject it.
    addCode(shader, QShader::MslShader, 23, msl, entry);
    return shader;
}

} // namespace miskeyed::workbench::slang_rhi::qt68
