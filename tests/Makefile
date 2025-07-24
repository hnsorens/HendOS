# Simple Makefile to compile all C files in current directory
BUILD_DIR := build
FILESYSTEM_DIR := ../filesystem/bin/tests
SRC_DIR := .
STD_LIB := ../std/lib/libstd.a

# Find all C files in current directory
C_FILES := $(wildcard $(SRC_DIR)/*.c)
PROGRAMS := $(patsubst $(SRC_DIR)/%.c,%,$(C_FILES))

# Compiler flags
CFLAGS := -D_FORTIFY_SOURCE=0 -fno-stack-protector -ffreestanding -nostdlib -nodefaultlibs -nostartfiles -static -fno-pie -no-pie -Os -s -I../std/include -L../crt -lcrt -Wall -Wextra
LDFLAGS := -static

.PHONY: all clean

all: clean $(patsubst %,$(FILESYSTEM_DIR)/%,$(PROGRAMS))
	@echo "All programs built and installed successfully"

$(FILESYSTEM_DIR)/%: $(BUILD_DIR)/%
	@mkdir -p $(FILESYSTEM_DIR)
	cp $< $@
	@echo "Installed: $@"

$(BUILD_DIR)/%: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "Building: $*"
	gcc $(CFLAGS) -o $@ $< $(LDFLAGS) $(STD_LIB)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(FILESYSTEM_DIR)
	@echo "Cleaned build artifacts"

list:
	@echo "Available programs:"
	@for prog in $(PROGRAMS); do echo "  $$prog"; done