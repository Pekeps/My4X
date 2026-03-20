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
    game/test_save_load.cpp
    game/test_faction.cpp
    game/test_faction_registry.cpp
    game/test_diplomacy_manager.cpp
    game/test_unit_template.cpp
    game/test_neutral_ai.cpp
    game/test_unit_production.cpp
    engine/test_hex_grid.cpp
    engine/test_faction_colors.cpp
    engine/test_diplomacy_colors.cpp
    game/test_faction_ui_data.cpp
    game/test_combat.cpp
    game/test_combat_log.cpp
    game/test_attack_action.cpp
    game/test_unit_experience.cpp
    game/test_unit_death_cleanup.cpp
)

target_link_libraries(my4x_tests PRIVATE my4x_game my4x_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(my4x_tests)

if(MSVC)
    target_compile_options(my4x_tests PRIVATE /W4)
else()
    target_compile_options(my4x_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
