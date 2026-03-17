# My4X Project Setup Design

## Overview

Set up a C++23 / raylib 4X game project with a modular CMake build system, GoogleTest testing, clang-tidy + clang-format linting, and GitHub Actions CI for Linux and Windows.

## Project Structure

```
My4X/
├── CMakeLists.txt              # Root CMake: C++23, FetchContent deps
├── .clang-format               # Code formatting rules
├── .clang-tidy                 # Static analysis checks
├── .gitignore
├── .github/
│   └── workflows/
│       └── ci.yml              # Build + test + lint (Linux & Windows)
│
├── engine/                     # Raylib wrapper: rendering, input, audio, window
│   ├── CMakeLists.txt          # Builds libengine, links raylib
│   ├── include/
│   │   └── engine/             # Public headers
│   └── src/
│
├── game/                       # Pure 4X logic: map, turns, resources, AI
│   ├── CMakeLists.txt          # Builds libgame, no raylib dependency
│   ├── include/
│   │   └── game/               # Public headers
│   └── src/
│
├── app/                        # Executable entry point
│   ├── CMakeLists.txt          # Links libengine + libgame
│   └── src/
│       └── main.cpp
│
└── tests/
    ├── CMakeLists.txt          # Links libgame + libengine + GTest
    ├── game/                   # Tests for game logic
    └── engine/                 # Tests for engine utilities
```

Key principle: `game/` has zero dependency on raylib. All rendering goes through `engine/`. Game logic is fully testable without a graphics context.

## CMake Architecture

### Root CMakeLists.txt

- `cmake_minimum_required(VERSION 3.24)`
- Project `My4X`, language CXX
- C++23 standard enforced globally (`CMAKE_CXX_STANDARD 23`, `CMAKE_CXX_STANDARD_REQUIRED ON`)
- FetchContent: raylib `5.5` (tag), GoogleTest `v1.15.2` (tag)
- `add_subdirectory()` for engine, game, app, tests
- Tests gated behind `BUILD_TESTING` option (ON by default)
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` for clang-tidy

### Module Targets

| Target | Type | Links to | Exposes headers via |
|--------|------|----------|-------------------|
| `my4x_engine` | STATIC library | raylib | `engine/include/` |
| `my4x_game` | STATIC library | nothing | `game/include/` |
| `my4x_app` | Executable | `my4x_engine`, `my4x_game` | — |
| `my4x_tests` | Executable | `my4x_game`, `my4x_engine`, GTest | — |

### Compiler Settings

- Warnings: `-Wall -Wextra -Wpedantic` on GCC/Clang, `/W4` on MSVC
- No special optimization flags in CMakeLists (left to build type)

## CI Pipeline

Single workflow `ci.yml` triggered on push and pull request to `main`.

### build-and-test (matrix: Linux, Windows)

| | Linux | Windows |
|---|---|---|
| OS | `ubuntu-latest` | `windows-latest` |
| Compiler | GCC 13 | MSVC (latest) |
| Generator | Ninja | Ninja |

Steps:
1. Checkout code
2. Install dependencies:
   - Linux: `ninja-build`, `libgl-dev`, `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, `libxkbcommon-dev`
   - Windows: Use `ilammy/msvc-dev-cmd@v1` action to set up MSVC environment, Ninja is pre-installed
3. Configure CMake (Release mode)
4. Build all targets
5. Run `ctest`

### lint (Linux only, parallel with builds)

Uses LLVM 18 (installed explicitly via `apt` for reproducibility).

Steps:
1. Checkout code
2. Install LLVM 18 (`clang-tidy-18`, `clang-format-18`)
3. Configure CMake to generate `compile_commands.json`
4. Run `clang-format-18 --dry-run --Werror` on all `*.cpp` and `*.h` files
5. Run `clang-tidy-18` on all source files using `compile_commands.json`

Both jobs must pass for the workflow to succeed.

## Linting Configuration

### .clang-format

- `BasedOnStyle: LLVM`
- `ColumnLimit: 120`
- `IndentWidth: 4`
- `UseTab: Never`
- `BreakBeforeBraces: Attach`
- `SortIncludes: CaseSensitive`

### .clang-tidy

- Checks: `bugprone-*`, `cppcoreguidelines-*`, `modernize-*`, `performance-*`, `readability-*`
- Disabled: `modernize-use-trailing-return-type`, `readability-magic-numbers`, `cppcoreguidelines-avoid-magic-numbers`
- `WarningsAsErrors: '*'`
- `HeaderFilterRegex: '.*/(engine|game)/include/.*'` (matches only project headers, not raylib/gtest)

### .gitignore

- Build dirs: `build/`, `cmake-build-*/`, `.cache/`
- Generated: `compile_commands.json`
- IDE: `.idea/`, `.vscode/`, `.vs/`
- OS: `.DS_Store`, `Thumbs.db`

## Starter Code

Minimal code to verify the full pipeline end-to-end.

### app/src/main.cpp

Creates a raylib window (800x600), runs a basic loop that clears the screen and draws "My4X" text, closes on window close. Uses `Engine` from the engine module.

### engine/include/engine/Engine.h + engine/src/Engine.cpp

`Engine` class with `init(width, height, title)`, `shutdown()`, `isRunning()`, `beginFrame()`, `endFrame()` wrapping raylib's InitWindow/CloseWindow/BeginDrawing/EndDrawing.

### game/include/game/GameState.h + game/src/GameState.cpp

`GameState` class with a `turn` counter, `nextTurn()` method, `getTurn()` getter. Pure logic, no raylib dependency.

### tests/game/test_game_state.cpp

GoogleTest: creates GameState, verifies initial turn is 1, calls nextTurn(), verifies turn is 2.

### tests/engine/ (placeholder)

Directory created with a `.gitkeep` file. Engine tests will be added as the engine API grows; no starter engine test since engine functionality requires a graphics context.
