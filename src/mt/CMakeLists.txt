set(MT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/mt)

function(configure_common)

set(sources
        ast.hpp
        ast_gen.hpp
        ast_gen.cpp
        character.hpp
        character.cpp
        character_traits.hpp
        error.hpp
        error.cpp
        FunctionRegistry.hpp
        FunctionRegistry.cpp
        identifier_classification.hpp
        identifier_classification.cpp
        lang_components.hpp
        lang_components.cpp
        keyword.hpp
        keyword.cpp
        Optional.hpp
        Result.hpp
        scan.hpp
        scan.cpp
        string.hpp
        string.cpp
        token.hpp
        token.cpp
        token_manipulation.hpp
        token_manipulation.cpp
        token_type.hpp
        token_type.cpp
        unicode.hpp
        unicode.cpp
)

foreach(source ${sources})
  target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/${source})
endforeach()

endfunction(configure_common)

configure_common()

add_subdirectory(ast)