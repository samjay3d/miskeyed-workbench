#include <slang_qrhi/anari/AnariDeviceCatalog.h>

#include <utility>

namespace slang_qrhi::anari_backend {

AnariDeviceCatalog::AnariDeviceCatalog(std::vector<std::string> candidates)
    : m_candidates(std::move(candidates))
{
}

void AnariDeviceCatalog::probe()
{
    m_entries.clear();
    m_entries.reserve(m_candidates.size());

    for (const auto& candidate : m_candidates) {
        AnariCandidateInfo info;
        info.candidate = candidate;

        auto library = AnariLibrary::load(candidate, &info.status);
        if (library) {
            info.available = true;
            for (const auto& subtype : library->deviceSubtypes())
                info.subtypes.push_back({ subtype, library->deviceExtensions(subtype) });
            info.status = library->statusMessages();
        }

        m_entries.push_back(std::move(info));
    }
}

} // namespace slang_qrhi::anari_backend
