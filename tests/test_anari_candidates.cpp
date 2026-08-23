#include <slang_qrhi/anari/AnariCandidates.h>

#include <cassert>
#include <vector>

int main()
{
    using slang_qrhi::anari_backend::parseCandidateList;

    assert((parseCandidateList("") == std::vector<std::string> {}));
    assert(
        (parseCandidateList("helide;visrtx") == std::vector<std::string> { "helide", "visrtx" }));
    assert((parseCandidateList(" helide ; visrtx,C:/sdk ; helide ; ;")
        == std::vector<std::string> { "helide", "visrtx,C:/sdk" }));
    return 0;
}
