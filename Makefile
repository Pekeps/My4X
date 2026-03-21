BUILD_DIR := build
BUILD_TYPE ?= Debug
JOBS ?= $(shell nproc)
SOURCES := $(shell find engine game network app server -name '*.cpp' -o -name '*.h')
CMAKE_FLAGS := -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

.PHONY: build test lint format format-check clean configure ci

configure:
	cmake $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR) -- -j$(JOBS)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

lint: build
	run-clang-tidy -p $(BUILD_DIR) $(SOURCES)

format:
	clang-format -i $(SOURCES)

format-check:
	clang-format --dry-run --Werror $(SOURCES)

clean:
	rm -rf $(BUILD_DIR)

# Agent-friendly target: runs format, build, lint, test.
# Suppresses verbose output; only prints step name + errors on failure, or a summary on success.
ci:
	@failed=0; \
	echo "--- format ---"; \
	clang-format -i $(SOURCES); \
	out=$$(clang-format --dry-run --Werror $(SOURCES) 2>&1); \
	if [ $$? -ne 0 ]; then echo "FAIL: format"; echo "$$out"; failed=1; fi; \
	\
	echo "--- build ---"; \
	out=$$(cmake $(CMAKE_FLAGS) 2>&1 && cmake --build $(BUILD_DIR) -- -j$(JOBS) 2>&1); \
	if [ $$? -ne 0 ]; then echo "FAIL: build"; echo "$$out" | grep -E "error:|FAILED"; failed=1; fi; \
	\
	if [ $$failed -eq 0 ]; then \
	  echo "--- lint ---"; \
	  out=$$(run-clang-tidy -p $(BUILD_DIR) $(SOURCES) 2>&1); \
	  if echo "$$out" | grep -qE "warning:|error:"; then \
	    echo "FAIL: lint"; echo "$$out" | grep -E "warning:|error:"; failed=1; \
	  fi; \
	  \
	  echo "--- test ---"; \
	  out=$$(ctest --test-dir $(BUILD_DIR) --output-on-failure 2>&1); \
	  if [ $$? -ne 0 ]; then echo "FAIL: test"; echo "$$out"; failed=1; \
	  else echo "$$out" | tail -1; fi; \
	fi; \
	\
	if [ $$failed -ne 0 ]; then echo "=== FAILED ==="; exit 1; \
	else echo "=== ALL PASSED ==="; fi
