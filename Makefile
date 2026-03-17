BUILD_DIR := build
BUILD_TYPE ?= Debug
SOURCES := $(shell find engine game app -name '*.cpp' -o -name '*.h')

.PHONY: build test lint format format-check clean configure

configure:
	cmake -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

lint: configure
	run-clang-tidy -p $(BUILD_DIR) $(SOURCES)

format:
	clang-format -i $(SOURCES)

format-check:
	clang-format --dry-run --Werror $(SOURCES)

clean:
	rm -rf $(BUILD_DIR)
