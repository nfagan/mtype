function(configure_fs)

    set(sources
            code_file.hpp
            code_file.cpp
            directory.hpp
            directory.cpp
            file.hpp
            file.cpp
            path.hpp
            path.cpp
            )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/fs/${source})
    endforeach()

endfunction(configure_fs)

configure_fs()