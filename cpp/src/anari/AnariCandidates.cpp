#include <slang_qrhi/anari/AnariCandidates.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_set>
#include <utility>

namespace slang_qrhi::anari_backend {
namespace {

    std::string trim(std::string_view value)
    {
        const auto first = std::find_if_not(
            value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
        const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
            return std::isspace(c) != 0;
        }).base();
        return first < last ? std::string(first, last) : std::string();
    }

} // namespace

std::vector<std::string> parseCandidateList(std::string_view configured)
{
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    std::size_t start = 0;
    while (start <= configured.size()) {
        const auto separator = configured.find(';', start);
        const auto end = separator == std::string_view::npos ? configured.size() : separator;
        auto candidate = trim(configured.substr(start, end - start));
        if (!candidate.empty() && seen.insert(candidate).second)
            result.push_back(std::move(candidate));
        if (separator == std::string_view::npos)
            break;
        start = separator + 1;
    }

    return result;
}

std::vector<std::string> configuredCandidates()
{
    if (const char* configured = std::getenv("MISKEYED_ANARI_LIBRARIES")) {
        auto candidates = parseCandidateList(configured);
        if (!candidates.empty())
            return candidates;
    }

    return { "helide", "helide_gpu", "visrtx", "debug" };
}

} // namespace slang_qrhi::anari_backend
