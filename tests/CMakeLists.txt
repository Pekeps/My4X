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
    engine/test_combat_effects.cpp
    engine/test_particle_system.cpp
    engine/test_transition.cpp
    engine/test_menu_system.cpp
    engine/test_model_manager.cpp
    game/test_faction_ui_data.cpp
    game/test_combat.cpp
    game/test_combat_log.cpp
    game/test_attack_action.cpp
    game/test_unit_experience.cpp
    game/test_unit_death_cleanup.cpp
    game/test_fog_of_war.cpp
    game/test_capture_action.cpp
    game/test_terrain_movement.cpp
    game/test_terrain_defense.cpp
    game/test_map_generator.cpp
    game/test_pathfinding.cpp
    game/test_movement_range.cpp
    game/test_zone_of_control.cpp
    game/test_game_session.cpp
    game/test_reconnection_manager.cpp
    game/test_turn_manager.cpp
    game/test_lobby_manager.cpp
    game/test_hex_direction.cpp
    engine/test_animation_controller.cpp
    engine/test_audio_manager.cpp
    engine/test_camera_cinematic.cpp
    engine/test_unit_animator.cpp
    engine/test_hex_mesh_builder.cpp
)

target_link_libraries(my4x_tests PRIVATE my4x_game my4x_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(my4x_tests)

if(MSVC)
    target_compile_options(my4x_tests PRIVATE /W4)
else()
    target_compile_options(my4x_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Network tests (only when gRPC is available) ─────────────────────────────
if(TARGET my4x_network)
    add_executable(my4x_network_tests
        network/test_network_client.cpp
    )

    target_link_libraries(my4x_network_tests PRIVATE my4x_network GTest::gtest_main)

    gtest_discover_tests(my4x_network_tests)

    if(MSVC)
        target_compile_options(my4x_network_tests PRIVATE /W4)
    else()
        target_compile_options(my4x_network_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
