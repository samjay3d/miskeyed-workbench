#pragma once

#include <anari/anari.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace slang_qrhi::anari_backend {

struct AnariStatusMessage {
    ANARIStatusSeverity severity = ANARI_SEVERITY_INFO;
    ANARIStatusCode code = ANARI_STATUS_NO_ERROR;
    ANARIDataType sourceType = ANARI_UNKNOWN;
    std::string text;
};

class AnariLibrary final {
public:
    static std::unique_ptr<AnariLibrary> load(
        std::string candidate, std::vector<AnariStatusMessage>* failureStatus = nullptr);

    ~AnariLibrary();
    AnariLibrary(const AnariLibrary&) = delete;
    AnariLibrary& operator=(const AnariLibrary&) = delete;

    ANARILibrary handle() const { return m_library; }
    const std::string& candidate() const { return m_candidate; }
    std::vector<std::string> deviceSubtypes() const;
    std::vector<std::string> deviceExtensions(const std::string& subtype) const;
    std::vector<AnariStatusMessage> statusMessages() const;

private:
    explicit AnariLibrary(std::string candidate);
    static void statusCallback(const void* userData, ANARIDevice, ANARIObject,
        ANARIDataType sourceType, ANARIStatusSeverity severity, ANARIStatusCode code,
        const char* message);

    std::string m_candidate;
    ANARILibrary m_library = nullptr;
    mutable std::mutex m_statusMutex;
    std::vector<AnariStatusMessage> m_status;
};

} // namespace slang_qrhi::anari_backend
