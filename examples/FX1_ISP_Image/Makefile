# Full tuning is always enabled by the application.
APP_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(APP_DIR)/build/FX1

.PHONY: all build clean

all: build

build:
	cd "$(APP_DIR)" && python3 tools/generate_profile.py
	cd "$(APP_DIR)" && cmake --preset FX1
	cd "$(APP_DIR)" && cmake --build --preset FX1

# Remove compiler outputs and CMake cache; preserve tuning and run artifacts.
clean:
	cmake -E rm -rf "$(BUILD_DIR)"
