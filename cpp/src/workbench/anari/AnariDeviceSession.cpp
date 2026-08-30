#include <miskeyed/workbench/anari/AnariDeviceSession.h>

#include <algorithm>
#include <utility>

namespace miskeyed::workbench::anari_backend {

AnariDeviceSession::AnariDeviceSession(std::unique_ptr<AnariLibrary> library, std::string subtype)
    : m_library(std::move(library))
    , m_subtype(std::move(subtype))
{
}

std::unique_ptr<AnariDeviceSession> AnariDeviceSession::create(
    std::string candidate, std::string subtype)
{
    auto library = AnariLibrary::load(std::move(candidate));
    if (!library)
        return nullptr;

    const auto subtypes = library->deviceSubtypes();
    if (std::find(subtypes.begin(), subtypes.end(), subtype) == subtypes.end())
        return nullptr;

    auto result = std::unique_ptr<AnariDeviceSession>(
        new AnariDeviceSession(std::move(library), std::move(subtype)));
    result->m_extensions = result->m_library->deviceExtensions(result->m_subtype);
    result->m_device = anariNewDevice(result->m_library->handle(), result->m_subtype.c_str());
    if (!result->m_device)
        return nullptr;

    anariCommitParameters(result->m_device, result->m_device);
    return result;
}

AnariDeviceSession::~AnariDeviceSession()
{
    if (m_device)
        anariRelease(m_device, m_device);
    // m_library is declared before the device handle and is destroyed after this body,
    // keeping implementation code loaded until the device has been released.
}

} // namespace miskeyed::workbench::anari_backend
