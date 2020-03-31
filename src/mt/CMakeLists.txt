set(MT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/mt)

function(configure_common)

set(sources
        ast.hpp
        ast_gen.hpp
        ast_gen.cpp
        character.hpp
        character.cpp
        character_traits.hpp
        definitions.hpp
        definitions.cpp
        display.hpp
        display.cpp
        error.hpp
        error.cpp
        fs.hpp
        handles.hpp
        identifier.hpp
        identifier.cpp
        store.hpp
        store.cpp
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
        text.hpp
        text.cpp
        token.hpp
        token.cpp
        token_manipulation.hpp
        token_manipulation.cpp
        token_type.hpp
        token_type.cpp
        traversal.hpp
        traversal.cpp
        unicode.hpp
        unicode.cpp
        utility.hpp
)

foreach(source ${sources})
  target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/${source})
endforeach()

endfunction(configure_common)

configure_common()

add_subdirectory(ast)
add_subdirectory(fs)