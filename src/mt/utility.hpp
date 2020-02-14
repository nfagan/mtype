#pragma once

#ifdef _MSC_VER
#define MT_IS_MSVC (1)
#else
#define MT_IS_MSVC (0)
#endif

#if MT_IS_MSVC
#define MSVC_MISSING_NOEXCEPT
#else
#define MSVC_MISSING_NOEXCEPT noexcept
#endif