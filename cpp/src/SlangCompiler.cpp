#include <slang_qrhi/SlangCompiler.h>
#include <slang_qrhi/Digest.h>
#include "Qt68ShaderBridge.h"
#include <slang.h>
#include <slang-com-ptr.h>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <algorithm>

namespace slang_qrhi {
namespace {

QString blobText(slang::IBlob* blob)
{
    if (!blob || !blob->getBufferPointer() || blob->getBufferSize() == 0) return {};
    return QString::fromUtf8(static_cast<const char*>(blob->getBufferPointer()), qsizetype(blob->getBufferSize()));
}
QByteArray blobBytes(slang::IBlob* blob)
{
    if (!blob || !blob->getBufferPointer() || blob->getBufferSize() == 0) return {};
    return QByteArray(static_cast<const char*>(blob->getBufferPointer()), qsizetype(blob->getBufferSize()));
}

ParameterType reflectType(slang::TypeReflection* type)
{
    if (!type) return ParameterType::Unknown;
    const auto kind = type->getKind();
    if (kind == slang::TypeReflection::Kind::Scalar) {
        switch (type->getScalarType()) {
        case slang::TypeReflection::ScalarType::Float32: return ParameterType::Float;
        case slang::TypeReflection::ScalarType::Int32: return ParameterType::Int;
        case slang::TypeReflection::ScalarType::UInt32: return ParameterType::UInt;
        case slang::TypeReflection::ScalarType::Bool: return ParameterType::Bool;
        default: return ParameterType::Unknown;
        }
    }
    if (kind == slang::TypeReflection::Kind::Vector) {
        auto* e = type->getElementType();
        const int n = int(type->getElementCount());
        if (!e) return ParameterType::Unknown;
        const auto scalar = e->getScalarType();
        if (scalar == slang::TypeReflection::ScalarType::Float32) return n==2?ParameterType::Float2:n==3?ParameterType::Float3:n==4?ParameterType::Float4:ParameterType::Unknown;
        if (scalar == slang::TypeReflection::ScalarType::Int32) return n==2?ParameterType::Int2:n==3?ParameterType::Int3:n==4?ParameterType::Int4:ParameterType::Unknown;
        if (scalar == slang::TypeReflection::ScalarType::UInt32) return n==2?ParameterType::UInt2:n==3?ParameterType::UInt3:n==4?ParameterType::UInt4:ParameterType::Unknown;
    }
    return ParameterType::Unknown;
}

QVariant defaultFor(ParameterType t)
{
    switch (t) {
    case ParameterType::Float: return 0.0;
    case ParameterType::Int: return 0;
    case ParameterType::UInt: return 0u;
    case ParameterType::Bool: return false;
    case ParameterType::Float2: return QVariant::fromValue(QVector2D());
    case ParameterType::Float3: return QVariant::fromValue(QVector3D());
    case ParameterType::Float4: return QVariant::fromValue(QVector4D());
    default: return {};
    }
}

// Recursively walk a uniform struct layout, accumulating the byte offset from the
// constant-buffer base. Nested structs become parameter groups (labelled by the field
// name), so uniforms are visible no matter how they are nested — not just when declared
// flat at global scope. Compiler-internal `__`-prefixed fields are skipped.
void collectParameters(slang::TypeLayoutReflection* tl, qsizetype baseOffset,
                       const QString& group, QList<ParameterDescriptor>& out)
{
    if (!tl) return;
    const auto fieldCount = tl->getFieldCount();
    for (unsigned i = 0; i < unsigned(fieldCount); ++i) {
        auto* field = tl->getFieldByIndex(i);
        if (!field || !field->getVariable()) continue;
        auto* var = field->getVariable();
        const QString name = QString::fromUtf8(var->getName());
        if (name.isEmpty() || name.startsWith(QLatin1String("__"))) continue; // compiler-internal
        const qsizetype offset = baseOffset + qsizetype(field->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM));
        auto* fieldTL = field->getTypeLayout();
        if (!fieldTL) continue;
        const auto kind = fieldTL->getKind();
        if (kind == slang::TypeReflection::Kind::ConstantBuffer ||
            kind == slang::TypeReflection::Kind::ParameterBlock) {
            if (auto* elem = fieldTL->getElementTypeLayout()) collectParameters(elem, offset, name, out);
            continue;
        }
        if (kind == slang::TypeReflection::Kind::Struct) {
            collectParameters(fieldTL, offset, name, out);
            continue;
        }
        ParameterDescriptor d;
        d.name = name;
        d.label = name;
        d.group = group;
        d.type = reflectType(var->getType());
        if (d.type == ParameterType::Unknown) continue; // resources become a separate model in the next slice
        d.offset = offset;
        d.size = qsizetype(fieldTL->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM));
        d.defaultValue = defaultFor(d.type);
        d.step = (d.type == ParameterType::Float || d.type == ParameterType::Float2 || d.type == ParameterType::Float3 || d.type == ParameterType::Float4) ? QVariant(0.01) : QVariant(1);

        // Optional UI metadata attributes drive the inspector:
        //   [UIName("..")] [UIGroup("..")] [UIWidget("slider|drag|angle")]
        //   [UIRange(lo,hi)] [UIStep(s)] [UITooltip("..")] [UIUnits("..")]
        QString units;
        for (unsigned a = 0, n = var->getUserAttributeCount(); a < n; ++a) {
            auto* attr = var->getUserAttributeByIndex(a);
            if (!attr) continue;
            const QByteArray an(attr->getName());
            auto argStr = [&](unsigned idx) -> QString {
                size_t len = 0;
                const char* s = attr->getArgumentValueString(idx, &len);
                if (!s) return {};
                QString q = QString::fromUtf8(s, int(len));
                if (q.size() >= 2 && q.startsWith(QLatin1Char('"')) && q.endsWith(QLatin1Char('"')))
                    q = q.mid(1, q.size() - 2); // Slang may hand back the literal with quotes
                return q;
            };
            auto argNum = [&](unsigned idx, double& v) -> bool {
                float f = 0.0f;
                if (SLANG_SUCCEEDED(attr->getArgumentValueFloat(idx, &f))) { v = f; return true; }
                int iv = 0;
                if (SLANG_SUCCEEDED(attr->getArgumentValueInt(idx, &iv))) { v = iv; return true; }
                return false;
            };
            if (an == "UIName")         { const QString s = argStr(0); if (!s.isEmpty()) d.label = s; }
            else if (an == "UIGroup")   { const QString s = argStr(0); if (!s.isEmpty()) d.group = s; }
            else if (an == "UIWidget")  { d.widget = argStr(0); }
            else if (an == "UITooltip") { d.tooltip = argStr(0); }
            else if (an == "UIUnits")   { units = argStr(0); }
            else if (an == "UIStep")    { double s; if (argNum(0, s)) d.step = s; }
            else if (an == "UIRange")   { double lo, hi; if (argNum(0, lo)) d.minimum = lo; if (argNum(1, hi)) d.maximum = hi; }
        }
        if (!units.isEmpty()) d.label += QStringLiteral(" (") + units + QLatin1Char(')');

        out.push_back(std::move(d));
    }
}

QList<ParameterDescriptor> reflectParameters(slang::ProgramLayout* layout, qsizetype& byteSize)
{
    QList<ParameterDescriptor> out;
    byteSize = 0;
    if (!layout) return out;
    byteSize = qsizetype(layout->getGlobalConstantBufferSize());
    auto* globals = layout->getGlobalParamsVarLayout();
    if (!globals) return out;
    auto* tl = globals->getTypeLayout();
    if (!tl) return out;
    // Global `uniform` parameters are wrapped in an implicit constant buffer; unwrap to
    // the element struct layout so getFieldCount() sees the actual parameters.
    if (tl->getKind() == slang::TypeReflection::Kind::ConstantBuffer ||
        tl->getKind() == slang::TypeReflection::Kind::ParameterBlock) {
        if (auto* elem = tl->getElementTypeLayout()) tl = elem;
    }

    collectParameters(tl, 0, QStringLiteral("Parameters"), out);

    // Order real (letter-named) parameters ahead of any `_`-prefixed ones so the
    // inspector reads cleanly; stable to preserve reflection/group order otherwise.
    std::stable_sort(out.begin(), out.end(), [](const ParameterDescriptor& a, const ParameterDescriptor& b) {
        const int ra = (!a.name.isEmpty() && a.name.at(0) == QLatin1Char('_')) ? 1 : 0;
        const int rb = (!b.name.isEmpty() && b.name.at(0) == QLatin1Char('_')) ? 1 : 0;
        return ra < rb;
    });
    return out;
}

} // namespace

class SlangCompilerPrivate {
public:
    Slang::ComPtr<slang::IGlobalSession> global;
    QStringList searchPaths;

    SlangCompilerPrivate() { slang::createGlobalSession(global.writeRef()); }
};

SlangCompiler::SlangCompiler(QObject* parent) : QObject(parent), d(std::make_unique<SlangCompilerPrivate>()) {}
SlangCompiler::~SlangCompiler() = default;
void SlangCompiler::setSearchPaths(QStringList paths) { d->searchPaths = std::move(paths); }
QStringList SlangCompiler::searchPaths() const { return d->searchPaths; }

CompileResult SlangCompiler::compileFullscreen(const QString& source, const QString& virtualPath,
                                               const QString& vertexEntry, const QString& fragmentEntry)
{
    CompileResult result;
    if (!d->global) { result.diagnostics = QStringLiteral("Failed to create Slang global session."); return result; }

    // Targets 0..2 feed the QRhi QShader (SPIR-V binary, HLSL, Metal). Targets 3..4 are
    // display-only, so the editor can show generated GLSL and human-readable SPIR-V.
    slang::TargetDesc targets[5] = {};
    targets[0].format = SLANG_SPIRV;
    targets[0].profile = d->global->findProfile("spirv_1_5");
    targets[1].format = SLANG_HLSL;
    targets[1].profile = d->global->findProfile("sm_5_0");
    targets[2].format = SLANG_METAL;
    targets[2].profile = d->global->findProfile("metal_2_0");
    targets[3].format = SLANG_GLSL;
    targets[3].profile = d->global->findProfile("glsl_450");
    targets[4].format = SLANG_SPIRV_ASM;
    targets[4].profile = d->global->findProfile("spirv_1_5");

    QList<QByteArray> searchUtf8;
    QVector<const char*> searchPtrs;
    for (const auto& p : d->searchPaths) searchUtf8.push_back(p.toUtf8());
    for (const auto& p : searchUtf8) searchPtrs.push_back(p.constData());

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = targets;
    sessionDesc.targetCount = 5;
    sessionDesc.searchPaths = searchPtrs.data();
    sessionDesc.searchPathCount = searchPtrs.size();

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(d->global->createSession(sessionDesc, session.writeRef())) || !session) {
        result.diagnostics = QStringLiteral("Failed to create Slang session."); return result;
    }

    const auto src = source.toUtf8();
    const auto path = virtualPath.toUtf8();
    const auto revision = Digest::hash(src).hex().left(16).toUtf8();
    const QByteArray moduleName = QByteArray("sqr_user_") + revision;
    Slang::ComPtr<slang::IBlob> diagnostics;
    auto* module = session->loadModuleFromSourceString(moduleName.constData(), path.constData(), src.constData(), diagnostics.writeRef());
    result.diagnostics += blobText(diagnostics);
    if (!module) return result;

    Slang::ComPtr<slang::IEntryPoint> vs, fs;
    diagnostics.setNull();
    if (SLANG_FAILED(module->findAndCheckEntryPoint(vertexEntry.toUtf8().constData(), SLANG_STAGE_VERTEX, vs.writeRef(), diagnostics.writeRef()))) {
        result.diagnostics += blobText(diagnostics); return result;
    }
    diagnostics.setNull();
    if (SLANG_FAILED(module->findAndCheckEntryPoint(fragmentEntry.toUtf8().constData(), SLANG_STAGE_FRAGMENT, fs.writeRef(), diagnostics.writeRef()))) {
        result.diagnostics += blobText(diagnostics); return result;
    }

    slang::IComponentType* parts[] = {module, vs.get(), fs.get()};
    Slang::ComPtr<slang::IComponentType> composite;
    diagnostics.setNull();
    if (SLANG_FAILED(session->createCompositeComponentType(parts, 3, composite.writeRef(), diagnostics.writeRef()))) {
        result.diagnostics += blobText(diagnostics); return result;
    }
    Slang::ComPtr<slang::IComponentType> linked;
    diagnostics.setNull();
    if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef()))) {
        result.diagnostics += blobText(diagnostics); return result;
    }

    slang::ProgramLayout* layout = linked->getLayout(0);
    result.parameters = reflectParameters(layout, result.parameterByteSize);
    result.parameterBinding = layout ? int(layout->getGlobalConstantBufferBinding()) : 0;
    QByteArray reflectionBytes;
    for (const auto& p : result.parameters) {
        reflectionBytes += p.name.toUtf8() + ':' + QByteArray::number(int(p.type)) + ':' + QByteArray::number(p.offset) + ':' + QByteArray::number(p.size) + ';';
    }
    result.reflectionDigest = Digest::hash(reflectionBytes).bytes();

    auto compileStage = [&](int epIndex, QShader::Stage stage, const QString& entry, CompiledStage& out) -> bool {
        QByteArray codes[3];
        int produced = 0;
        // Target index -> display name for the generated-code viewer (index 0 is SPIR-V
        // binary, shown via its assembly form at index 4 instead).
        static const char* const kTargetName[5] = { nullptr, "HLSL", "Metal", "GLSL", "SPIR-V" };
        for (int target = 0; target < 5; ++target) {
            Slang::ComPtr<slang::IBlob> code, diag;
            // A backend can be unavailable (e.g. SPIR-V needs slang-glslang); skip it
            // instead of failing the whole compile so the active RHI still renders.
            if (SLANG_FAILED(linked->getEntryPointCode(epIndex, target, code.writeRef(), diag.writeRef()))) {
                // Only surface diagnostics from the targets that back the live pipeline;
                // the display-only targets must not spam the Diagnostics panel.
                if (target <= 2) result.diagnostics += blobText(diag);
                continue;
            }
            if (target <= 2) {
                result.diagnostics += blobText(diag);
                codes[target] = blobBytes(code);
                if (!codes[target].isEmpty()) ++produced;
            }
            if (kTargetName[target]) {
                const QString text = blobText(code);
                if (!text.isEmpty()) out.generated.insert(QString::fromLatin1(kTargetName[target]), text);
            }
        }
        if (produced == 0) return false;
        Slang::ComPtr<slang::IBlob> hash;
        linked->getEntryPointHash(epIndex, 0, hash.writeRef());
        out.entryPointHash = blobBytes(hash);
        out.shader = qt68::buildQShader(stage, entry, codes[0], codes[1], codes[2], layout, result.parameters);
        return out.shader.isValid();
    };

    if (!compileStage(0, QShader::VertexStage, vertexEntry, result.vertex)) return result;
    if (!compileStage(1, QShader::FragmentStage, fragmentEntry, result.fragment)) return result;
    result.ok = true;
    return result;
}

} // namespace slang_qrhi
