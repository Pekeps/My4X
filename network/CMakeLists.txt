# Network module — only built when gRPC is available.
if(TARGET my4x_grpc)
    add_library(my4x_network STATIC
        src/NetworkClient.cpp
    )

    target_include_directories(my4x_network PUBLIC include)
    target_link_libraries(my4x_network PUBLIC my4x_grpc)

    if(MSVC)
        target_compile_options(my4x_network PRIVATE /W4)
    else()
        target_compile_options(my4x_network PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    message(STATUS "Skipping network module — gRPC not available")
endif()
