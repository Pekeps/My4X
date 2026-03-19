add_library(my4x_game STATIC
    src/GameState.cpp
    src/Tile.cpp
    src/TileRegistry.cpp
    src/Map.cpp
    src/Unit.cpp
    src/Warrior.cpp
    src/Resource.cpp
    src/Building.cpp
    src/City.cpp
    src/Serialization.cpp
    src/BuildQueue.cpp
    src/TurnResolver.cpp
    src/SaveLoad.cpp
    src/Faction.cpp
)

target_include_directories(my4x_game PUBLIC include)
target_link_libraries(my4x_game PUBLIC my4x_proto)

if(MSVC)
    target_compile_options(my4x_game PRIVATE /W4)
else()
    target_compile_options(my4x_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
