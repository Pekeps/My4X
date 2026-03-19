add_executable(my4x_tests
    game/test_game_state.cpp
    game/test_tile_registry.cpp
    game/test_terrain.cpp
    game/test_unit.cpp
    game/test_resource.cpp
    game/test_building.cpp
    game/test_city.cpp
    game/test_serialization.cpp
    game/test_build_queue.cpp
    game/test_turn_resolver.cpp
    engine/test_hex_grid.cpp
)

target_link_libraries(my4x_tests PRIVATE my4x_game my4x_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(my4x_tests)

if(MSVC)
    target_compile_options(my4x_tests PRIVATE /W4)
else()
    target_compile_options(my4x_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
