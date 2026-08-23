#include <slang_qrhi/anari/AnariLibrary.h>

#include <utility>

namespace slang_qrhi::anari_backend {
namespace {

    std::vector<std::string> copyStringList(const char* const* values)
    {
        std::vector<std::string> result;
        for (auto value = values; value && *value; ++value)
            result.emplace_back(*value);
        return result;
    }

} // namespace

AnariLibrary::AnariLibrary(std::string candidate)
    : m_candidate(std::move(candidate))
{
}

std::unique_ptr<AnariLibrary> AnariLibrary::load(
    std::string candidate, std::vector<AnariStatusMessage>* failureStatus)
{
    auto result = std::unique_ptr<AnariLibrary>(new AnariLibrary(std::move(candidate)));
    result->m_library = anariLoadLibrary(
        result->m_candidate.c_str(), &AnariLibrary::statusCallback, result.get());
    if (!result->m_library) {
        if (failureStatus)
            *failureStatus = result->statusMessages();
        return nullptr;
    }
    return result;
}

AnariLibrary::~AnariLibrary()
{
    if (m_library)
        anariUnloadLibrary(m_library);
}

std::vector<std::string> AnariLibrary::deviceSubtypes() const
{
    return copyStringList(anariGetDeviceSubtypes(m_library));
}

std::vector<std::string> AnariLibrary::deviceExtensions(const std::string& subtype) const
{
    return copyStringList(anariGetDeviceExtensions(m_library, subtype.c_str()));
}

std::vector<AnariStatusMessage> AnariLibrary::statusMessages() const
{
    std::lock_guard lock(m_statusMutex);
    return m_status;
}

void AnariLibrary::statusCallback(const void* userData, ANARIDevice, ANARIObject,
    ANARIDataType sourceType, ANARIStatusSeverity severity, ANARIStatusCode code,
    const char* message)
{
    auto* self = static_cast<AnariLibrary*>(const_cast<void*>(userData));
    if (!self)
        return;
    std::lock_guard lock(self->m_statusMutex);
    self->m_status.push_back(
        { severity, code, sourceType, message ? std::string(message) : std::string() });
}

} // namespace slang_qrhi::anari_backend
