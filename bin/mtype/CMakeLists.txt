project(mtype)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        MT_MATLAB_DATA_DIR="${MT_PROJECT_SOURCE_DIR}/matlab/data")

target_sources(${PROJECT_NAME} PRIVATE
        app.hpp
        app.cpp
        ast_store.hpp
        ast_store.cpp
        main.cpp
        command_line.hpp
        command_line.cpp
        external_resolution.hpp
        external_resolution.cpp
        parse_pipeline.hpp
        parse_pipeline.cpp
        pre_imports.hpp
        pre_imports.cpp
        show.hpp
        show.cpp
        type_analysis.hpp
        type_analysis.cpp
        )

configure_compiler_flags()