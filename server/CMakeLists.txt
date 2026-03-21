# The server target is only built when gRPC is available (Linux).
if(TARGET my4x_grpc)
    add_executable(my4x_server
        src/main.cpp
        src/LobbyServiceImpl.cpp
        src/GameServiceImpl.cpp
    )

    target_include_directories(my4x_server PRIVATE src)
    target_link_libraries(my4x_server PRIVATE my4x_grpc my4x_game)

    if(MSVC)
        target_compile_options(my4x_server PRIVATE /W4)
    else()
        target_compile_options(my4x_server PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    message(STATUS "Skipping my4x_server — gRPC not available")
endif()
