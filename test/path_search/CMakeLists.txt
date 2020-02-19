project(path_search)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)
target_compile_definitions(${PROJECT_NAME} PUBLIC
        MT_MATLAB_DATA_DIR="${MT_PROJECT_SOURCE_DIR}/matlab/data")

target_sources(${PROJECT_NAME} PRIVATE main.cpp)