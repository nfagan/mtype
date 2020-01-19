function(configure_ast)

    set(sources
            ast.hpp
            def.hpp
            expr.hpp
            expr.cpp
            stmt.hpp
    )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/ast/${source})
    endforeach()

endfunction(configure_ast)

configure_ast()