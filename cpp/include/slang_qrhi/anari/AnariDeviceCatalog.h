#pragma once

#include <slang_qrhi/anari/AnariLibrary.h>

#include <string>
#include <vector>

namespace slang_qrhi::anari_backend {

struct AnariDeviceSubtypeInfo {
    std::string name;
    std::vector<std::string> extensions;
};

struct AnariCandidateInfo {
    std::string candidate;
    bool available = false;
    std::vector<AnariDeviceSubtypeInfo> subtypes;
    std::vector<AnariStatusMessage> status;
};

class AnariDeviceCatalog final {
public:
    explicit AnariDeviceCatalog(std::vector<std::string> candidates);

    void probe();
    const std::vector<AnariCandidateInfo>& entries() const { return m_entries; }

private:
    std::vector<std::string> m_candidates;
    std::vector<AnariCandidateInfo> m_entries;
};

} // namespace slang_qrhi::anari_backend
