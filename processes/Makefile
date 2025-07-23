# processes/Makefile - Fixed version
BUILD_DIR := build
FILESYSTEM_DIR := ../filesystem/bin
SRC_DIR := src
INCLUDE_DIR := include
STD_LIB := ../std/lib/libstd.a

# Safely find all programs with src/main.c
# PROGRAMS := $(shell find . -mindepth 2 -name "main.c" -path "*/$(SRC_DIR)/main.c" -exec dirname {} \; | xargs -I{} dirname {} | sort -u)
PROGRAMS := $(shell find . -mindepth 1 -type d -name $(SRC_DIR) | xargs -I{} dirname {} | sort -u)

# Compiler flags
CFLAGS := -D_FORTIFY_SOURCE=0 -fno-stack-protector -ffreestanding -nostdlib -nodefaultlibs -nostartfiles -static -fno-pie -no-pie -Os -s -o -I$(INCLUDE_DIR) -I../std/include -L../crt -lcrt -Wall -Wextra
LDFLAGS := -static

.PHONY: all clean

all: clean $(patsubst %,$(FILESYSTEM_DIR)/%,$(notdir $(PROGRAMS)))
	@echo "Processes built successfully"

$(FILESYSTEM_DIR)/%: $(BUILD_DIR)/%
	@mkdir -p $(FILESYSTEM_DIR)
	cp $< $@
	@echo "Installed: $@"

$(BUILD_DIR)/%: %
	@mkdir -p $(@D)
	@echo "Building: $(notdir $*)"
	gcc $(CFLAGS) -o $@ $(shell find $*/$(SRC_DIR) -name "*.c") $(LDFLAGS) $(STD_LIB)

clean:
	rm -rf ../filesystem/bin
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts"

list:
	@echo "Available programs:"
	@for prog in $(notdir $(PROGRAMS)); do echo "  $$prog"; done