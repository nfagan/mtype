function(configure_scan)

    set(sources
            scan.hpp
            scan.cpp
            token_manipulation.hpp
            token_manipulation.cpp
            )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/scan/${source})
    endforeach()

endfunction(configure_scan)

configure_scan()