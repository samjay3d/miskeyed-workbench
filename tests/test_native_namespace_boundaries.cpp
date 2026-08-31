#include <miskeyed/workbench/anari/AnariCandidates.h>
#include <miskeyed/workbench/slang/ShaderDocument.h>

#include <type_traits>

int main()
{
    using RendererDocument = miskeyed::workbench::slang_rhi::ShaderDocument;
    using CandidateList
        = decltype(miskeyed::workbench::anari_backend::parseCandidateList(std::string_view {}));

    static_assert(std::is_class_v<RendererDocument>);
    static_assert(std::is_default_constructible_v<CandidateList>);
    return 0;
}
