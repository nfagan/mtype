function(configure_ast)

    set(sources
            ast.hpp
            def.hpp
            def.cpp
            expr.hpp
            expr.cpp
            stmt.hpp
            stmt.cpp
            type_annot.hpp
            type_annot.cpp
            StringVisitor.hpp
            StringVisitor.cpp
    )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/ast/${source})
    endforeach()

endfunction(configure_ast)

configure_ast()