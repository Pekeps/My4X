# Project Instructions for AI Assistants

## Build Commands

Always use the top-level Makefile for all build operations:

- **Build:** `make build`
- **Test:** `make test`
- **Lint:** `make lint`
- **Format:** `make format`
- **Format check:** `make format-check`
- **Clean:** `make clean`

Do NOT invoke cmake, ninja, or ctest directly. Do NOT run test binaries directly.

## Coding Standards

- **No magic numbers.** Clang-tidy enforces `cppcoreguidelines-avoid-magic-numbers`. Use named `static constexpr` constants for any numeric literal in game logic (thresholds, costs, stats, formulas). Bare literals like `10`, `5`, `100` in logic code will fail CI. Test files and `main.cpp` use `NOLINTBEGIN`/`NOLINTEND` blocks to suppress this where appropriate.

## Agent Subtask Workflow

When an agent is dispatched to implement a subtask, it MUST follow this workflow before reporting done:

1. Implement the feature on a dedicated branch
2. Run `make format && make build && make test` locally — all must pass
3. Push the branch and create a PR via `gh pr create`
4. Wait for CI to finish: `gh pr checks <pr-number> --watch --fail-fast`
5. If CI fails, read the failing logs (`gh run view <run-id> --log-failed`), fix the issue, push, and repeat from step 4
6. Only report done once CI is fully green

Branch naming convention: `<LINEAR-ID>/<short-description>` (e.g. `TON-20/turn-resolver-for-economy`)
