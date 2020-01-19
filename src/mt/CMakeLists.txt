set(MT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/mt)

function(configure_common)

set(sources
        ast.hpp
        ast_gen.hpp
        ast_gen.cpp
        character.hpp
        character.cpp
        character_traits.hpp
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
        token_type.hpp
        token_type.cpp
        token.hpp
        token.cpp
        unicode.hpp
        unicode.cpp
)

foreach(source ${sources})
  target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/${source})
endforeach()

endfunction(configure_common)

configure_common()

add_subdirectory(ast)