add_executable(my4x_app
    src/main.cpp
)

target_link_libraries(my4x_app PRIVATE my4x_engine my4x_game)

if(MSVC)
    target_compile_options(my4x_app PRIVATE /W4)
else()
    target_compile_options(my4x_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
