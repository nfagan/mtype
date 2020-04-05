project(types1)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        MT_MATLAB_DATA_DIR="${MT_PROJECT_SOURCE_DIR}/matlab/data")

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        test_cases.hpp
        test_cases.cpp
        util.hpp
        util.cpp
        )