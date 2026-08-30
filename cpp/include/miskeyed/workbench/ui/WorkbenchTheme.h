#pragma once
#include <miskeyed/workbench/Export.h>
class QWidget;
namespace miskeyed::workbench::ui {
// A product style avoids platform-native control metrics changing the workbench layout.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT WorkbenchTheme final {
public:
    static void apply(QWidget& root);
};
}
