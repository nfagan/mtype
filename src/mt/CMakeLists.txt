set(MT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/mt)

function(configure_common)

set(sources
        ast.hpp
        character.hpp
        character.cpp
        character_traits.hpp
        config.hpp
        definitions.hpp
        definitions.cpp
        display.hpp
        display.cpp
        error.hpp
        error.cpp
        error_filter.hpp
        error_filter.cpp
        fs.hpp
        handles.hpp
        identifier.hpp
        identifier.cpp
        store.hpp
        store.cpp
        lang_components.hpp
        lang_components.cpp
        keyword.hpp
        keyword.cpp
        Optional.hpp
        Result.hpp
        parse.hpp
        scan.hpp
        search_path.hpp
        search_path.cpp
        string.hpp
        string.cpp
        text.hpp
        text.cpp
        token.hpp
        token.cpp
        type.hpp
        source_data.hpp
        source_data.cpp
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
add_subdirectory(parse)
add_subdirectory(scan)
add_subdirectory(type)