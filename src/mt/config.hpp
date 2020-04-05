#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  #define MT_WIN
#elif __APPLE__
  #define MT_MACOS
  #define MT_UNIX
#elif __linux__
  #define MT_LINUX
  #define MT_UNIX
#elif __unix__
  #define MT_LINUX
  #define MT_UNIX
#endif

#ifdef _MSC_VER
#define MT_IS_MSVC (1)
#else
#define MT_IS_MSVC (0)
#endif

#ifndef NDEBUG
#define MT_DEBUG
#endif

#if MT_IS_MSVC
#define MSVC_MISSING_NOEXCEPT
#else
#define MSVC_MISSING_NOEXCEPT noexcept
#endif