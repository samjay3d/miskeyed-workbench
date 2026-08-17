#pragma once
#include <QtCore/qglobal.h>

#if defined(SLANG_QRHI_STATIC)
#define SLANG_QRHI_EXPORT
#elif defined(SLANG_QRHI_LIBRARY)
#define SLANG_QRHI_EXPORT Q_DECL_EXPORT
#else
#define SLANG_QRHI_EXPORT Q_DECL_IMPORT
#endif
