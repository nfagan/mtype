#pragma once

#ifdef _MSC_VER
#define MSVC_MISSING_NOEXCEPT
#else
#define MSVC_MISSING_NOEXCEPT noexcept
#endif