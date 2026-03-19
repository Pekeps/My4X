# Project Instructions for AI Assistants

## Build Commands

Always use the top-level Makefile for all build operations:

- **Build:** `make build`
- **Test:** `make test`
- **Lint:** `make lint`
- **Format:** `make format`
- **Format check:** `make format-check`
- **CI (agents):** `make ci` — runs format, build, lint, test with minimal output (errors only on failure)
- **Clean:** `make clean`

Do NOT invoke cmake, ninja, or ctest directly. Do NOT run test binaries directly.

## Coding Standards

- **No magic numbers.** Clang-tidy enforces `cppcoreguidelines-avoid-magic-numbers`. Use named `static constexpr` constants for any numeric literal in game logic (thresholds, costs, stats, formulas). Bare literals like `10`, `5`, `100` in logic code will fail CI. Test files and `main.cpp` use `NOLINTBEGIN`/`NOLINTEND` blocks to suppress this where appropriate.

## Agent Subtask Workflow

When an agent is dispatched to implement a subtask, it MUST follow this workflow before reporting done:

1. Implement the feature on a dedicated branch
2. Run `make ci` locally — all steps must pass
3. Before pushing, rebase onto latest master to avoid merge conflicts:
   ```
   git fetch origin && git rebase origin/master
   ```
   If there are conflicts, resolve them, then `git add` the resolved files and `git rebase --continue`. After resolving, re-run `make ci` to verify nothing broke.
4. Push the branch and create a PR via `gh pr create`
5. Wait for CI to finish: `gh pr checks <pr-number> --watch --fail-fast`
6. If CI fails, read the failing logs (`gh run view <run-id> --log-failed`), fix the issue, push, and repeat from step 5
7. If the PR shows merge conflicts (CI may not run), rebase onto `origin/master`, resolve conflicts, re-run `make ci`, and force-push: `git push --force-with-lease`
8. Only report done once CI is fully green

Branch naming convention: `<LINEAR-ID>/<short-description>` (e.g. `TON-20/turn-resolver-for-economy`)
