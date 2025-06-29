.PHONY: all clean

all: clean lib/libstd.a
	@echo "Static library built: lib/libstd.a"

lib/libstd.a: \
  $(shell find src -name "*.c" | sed 's|^src/|build/|' | sed 's|.c$$|.o|')
	mkdir -p lib
	ar rcs lib/libstd.a $(shell find build -name "*.o")

# Rule to compile each .c to .o in build/
build/%.o: src/%.c
	mkdir -p $(dir $@)
	gcc -ffreestanding -nostdlib -nodefaultlibs -nostartfiles -Wall -Wextra -c $< -o $@
	@echo "Compiled: $< -> $@"

clean:
	rm -rf build lib
	@echo "Cleaned build artifacts"
