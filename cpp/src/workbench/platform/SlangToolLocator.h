#pragma once

#include <QString>

namespace miskeyed::workbench::platform {

// Runtime tool lookup is an operating-system edge, not a WorkbenchWindow concern.
[[nodiscard]] QString findSlangLanguageServer();

} // namespace miskeyed::workbench::platform
