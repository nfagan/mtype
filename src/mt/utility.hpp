#pragma once

#ifdef _MSVC
#define MSVC_MISSING_NOEXCEPT
#else
#define MSVC_MISSING_NOEXCEPT noexcept
#endif