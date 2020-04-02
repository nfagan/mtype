project(types1)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        debug.hpp
        error.hpp
        error.cpp
        instance.hpp
        instance.cpp
        library.hpp
        library.cpp
        member_visitor.hpp
        member_visitor.cpp
        simplify.hpp
        simplify.cpp
        substitution.hpp
        substitution.cpp
        test_cases.hpp
        test_cases.cpp
        type.hpp
        type.cpp
        type_constraint_gen.hpp
        type_constraint_gen.cpp
        type_properties.hpp
        type_properties.cpp
        type_relation.hpp
        type_relation.cpp
        type_relationships.hpp
        type_relationships.cpp
        type_representation.hpp
        type_representation.cpp
        type_store.hpp
        type_store.cpp
        unification.hpp
        unification.cpp
        util.hpp
        util.cpp
        )

#if(MSVC)
#    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
#elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
#else()
#    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
#endif()