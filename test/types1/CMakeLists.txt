project(types1)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        debug.hpp
        debug.cpp
        test_cases.hpp
        test_cases.cpp
        type.hpp
        type.cpp
        typing.hpp
        typing.cpp
        type_store.hpp
        type_store.cpp
        type_equality.hpp
        type_equality.cpp
        unification.hpp
        unification.cpp
        util.hpp
        util.cpp
        )