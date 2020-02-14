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

#define MT_DELETE_COPY_CTOR_AND_ASSIGNMENT(class_name) \
  class_name(const class_name& other) = delete; \
  class_name& operator=(const class_name& other) = delete;