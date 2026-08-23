#include <miskeyed/workbench/anari/AnariCandidates.h>
#include <miskeyed/workbench/anari/AnariDeviceCatalog.h>
#include <miskeyed/workbench/anari/AnariDeviceSession.h>

#include <iostream>

int main(int argc, char** argv)
{
    using namespace miskeyed::workbench::anari_backend;

    const auto candidates = argc > 1 ? parseCandidateList(argv[1]) : configuredCandidates();
    AnariDeviceCatalog catalog(candidates);
    catalog.probe();

    bool loadedAny = false;
    for (const auto& entry : catalog.entries()) {
        if (!entry.available) {
            std::cerr << entry.candidate << ": unavailable\n";
            for (const auto& status : entry.status)
                std::cerr << "  " << status.text << '\n';
            continue;
        }

        std::cout << entry.candidate << '\n';
        for (const auto& subtype : entry.subtypes) {
            auto session = AnariDeviceSession::create(entry.candidate, subtype.name);
            std::cout << "  subtype=" << subtype.name
                      << " device=" << (session ? "ok" : "unavailable") << '\n';
            loadedAny = loadedAny || bool(session);
            for (const auto& extension : subtype.extensions)
                std::cout << "    " << extension << '\n';
        }
    }
    return loadedAny ? 0 : 1;
}
