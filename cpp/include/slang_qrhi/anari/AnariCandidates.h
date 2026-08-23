#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace slang_qrhi::anari_backend {

// Candidate discovery is application policy, not an ANARI registry. Semicolons
// separate candidates because the ANARI loader already uses a comma in "name,path".
std::vector<std::string> parseCandidateList(std::string_view configured);
std::vector<std::string> configuredCandidates();

} // namespace slang_qrhi::anari_backend
