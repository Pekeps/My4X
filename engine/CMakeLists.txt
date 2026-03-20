add_library(my4x_engine STATIC
    src/Window.cpp
    src/HexGrid.cpp
    src/MapRenderer.cpp
    src/UnitRenderer.cpp
    src/CityRenderer.cpp
    src/BuildingRenderer.cpp
    src/Camera.cpp
    src/Input.cpp
    src/CombatEffects.cpp
    src/CombatLogRenderer.cpp
    src/RangeOverlay.cpp
    src/Minimap.cpp
    src/AnimationController.cpp
    src/ParticleSystem.cpp
    src/ModelManager.cpp
)

target_include_directories(my4x_engine PUBLIC include)
target_link_libraries(my4x_engine PUBLIC raylib my4x_game)

if(MSVC)
    target_compile_options(my4x_engine PRIVATE /W4)
else()
    target_compile_options(my4x_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
