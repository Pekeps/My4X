cmake_minimum_required(VERSION 3.24)
project(My4X LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Reuse cached sources when available (avoids re-cloning in worktrees / parallel agents).
set(_DEPS_CACHE "$ENV{HOME}/.cache/cmake-fetchcontent")
if(EXISTS "${_DEPS_CACHE}/raylib-src/CMakeLists.txt" AND NOT FETCHCONTENT_SOURCE_DIR_RAYLIB)
    set(FETCHCONTENT_SOURCE_DIR_RAYLIB "${_DEPS_CACHE}/raylib-src")
endif()
if(EXISTS "${_DEPS_CACHE}/googletest-src/CMakeLists.txt" AND NOT FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
    set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST "${_DEPS_CACHE}/googletest-src")
endif()

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)

# SQLite3 — prefer system-installed library, fall back to amalgamation.
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "System SQLite3 not found — fetching amalgamation")
    if(EXISTS "${_DEPS_CACHE}/sqlite-src/sqlite3.c" AND NOT FETCHCONTENT_SOURCE_DIR_SQLITE)
        set(FETCHCONTENT_SOURCE_DIR_SQLITE "${_DEPS_CACHE}/sqlite-src")
    endif()
    FetchContent_Declare(
        sqlite
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
    )
    FetchContent_MakeAvailable(sqlite)
    add_library(sqlite3 STATIC ${sqlite_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC ${sqlite_SOURCE_DIR})
    set_target_properties(sqlite3 PROPERTIES C_STANDARD 11)
    if(NOT MSVC)
        target_compile_options(sqlite3 PRIVATE -w)
    endif()
else()
    message(STATUS "Using system SQLite3 ${SQLite3_VERSION}")
    # Create an alias so downstream can link to "sqlite3" uniformly.
    if(NOT TARGET sqlite3)
        add_library(sqlite3 ALIAS SQLite::SQLite3)
    endif()
endif()

FetchContent_MakeAvailable(raylib)

add_subdirectory(engine)
add_subdirectory(proto)
add_subdirectory(game)
add_subdirectory(server)
add_subdirectory(network)
add_subdirectory(app)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()
