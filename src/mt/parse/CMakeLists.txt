function(configure_parse)

    set(sources
            ast_gen.hpp
            ast_gen.cpp
            external_visibility.hpp
            external_visibility.cpp
            identifier_classification.hpp
            identifier_classification.cpp
            )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/parse/${source})
    endforeach()

endfunction(configure_parse)

configure_parse()