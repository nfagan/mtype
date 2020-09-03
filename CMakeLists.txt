cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

project(mt)

add_library(${PROJECT_NAME} STATIC)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

if (MSVC)
  target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/deps/dirent/include
  )
endif()

set(MT_PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC
  MT_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

function(configure_compiler_flags)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
  endif()
endfunction(configure_compiler_flags)

configure_compiler_flags()

add_subdirectory(src/mt)
add_subdirectory(test)
add_subdirectory(bin)
