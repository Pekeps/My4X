add_library(my4x_engine STATIC
    src/Window.cpp
)

target_include_directories(my4x_engine PUBLIC include)
target_link_libraries(my4x_engine PUBLIC raylib)

if(MSVC)
    target_compile_options(my4x_engine PRIVATE /W4)
else()
    target_compile_options(my4x_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
