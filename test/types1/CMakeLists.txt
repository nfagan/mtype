project(types1)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} mt)

target_sources(${PROJECT_NAME} PRIVATE
        main.cpp
        util.hpp
        util.cpp
        )