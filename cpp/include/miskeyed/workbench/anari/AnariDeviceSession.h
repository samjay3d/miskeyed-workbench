#pragma once

#include <miskeyed/workbench/anari/AnariLibrary.h>

#include <memory>
#include <string>
#include <vector>

namespace miskeyed::workbench::anari_backend {

class AnariDeviceSession final {
public:
    static std::unique_ptr<AnariDeviceSession> create(
        std::string candidate, std::string subtype = "default");

    ~AnariDeviceSession();
    AnariDeviceSession(const AnariDeviceSession&) = delete;
    AnariDeviceSession& operator=(const AnariDeviceSession&) = delete;

    ANARIDevice device() const { return m_device; }
    const AnariLibrary& library() const { return *m_library; }
    const std::string& subtype() const { return m_subtype; }
    const std::vector<std::string>& extensions() const { return m_extensions; }

private:
    AnariDeviceSession(std::unique_ptr<AnariLibrary> library, std::string subtype);

    std::unique_ptr<AnariLibrary> m_library;
    std::string m_subtype;
    std::vector<std::string> m_extensions;
    ANARIDevice m_device = nullptr;
};

} // namespace miskeyed::workbench::anari_backend
