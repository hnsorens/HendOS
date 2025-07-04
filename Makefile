.PHONY: all clean run

all:

	@echo "Building Standard Library..."
	@$(MAKE) -C std
	@echo "Standard Library built successfully"

	@echo "Building all processes..."
	@$(MAKE) -C processes
	@echo "Processes built successfully"

	@echo "Building C Runtime."
	@$(MAKE) -C crt
	@echo "C Runtime built successfully"

	# Clean previous builds
	rm -f build/main.efi *.o
	rm -rf image
	rm -f build/disk.img
	rm -rf mnt

	# Build EFI executable
	mkdir -p build
	x86_64-w64-mingw32-gcc -fPIC -pie -m64 -mno-red-zone -fno-stack-protector -w -nostdlib -Ignu-efi/inc -Ignu-efi/inc/x86_64 -Ignu-efi/inc/protocol -DCONFIG_x86_64 -D__MAKEWITH_GNUEFI -DGNU_EFI_USE_MS_ABI -D_DEBUG -ffreestanding -c -Iinclude/kstd -Iinclude/std -Iinclude $$(find src -name '*.c')
	nasm -f elf64 src/arch/interrupts.asm -o interrupts.o
	x86_64-w64-mingw32-gcc -Wl,--image-base=0x00000FFF00000000 -fPIC -pie -Wl,-dll -Wl,--subsystem,10 -Lgnu-efi/x86_64/lib -e efi_main -s -Wl,-Bsymbolic -nostdlib -shared $$(find src -name '*.c' | sed 's|.*/||; s/\.c$$/.o/') interrupts.o -o build/main.efi -lefi
	find . -maxdepth 2 -type f -name '*.o' -delete
	mkdir -p image/efi/boot
	cp -f build/main.efi image/efi/boot/bootx64.efi

	# Create disk image
	dd if=/dev/zero of=build/disk.img bs=1M count=128
	sudo parted build/disk.img --script mklabel gpt
	sudo parted build/disk.img --script mkpart primary fat32 1MiB 64MiB
	sudo parted build/disk.img --script mkpart primary 64MiB 100%
	LOOPDEV=$$(sudo losetup --find --partscan --show build/disk.img); \
	sudo mkfs.fat -F32 "$${LOOPDEV}p1"; \
	sudo mkfs.ext2 "$${LOOPDEV}p2"; \
	mkdir -p mnt1 mnt2; \
	sudo mount "$${LOOPDEV}p1" mnt1; \
	sudo mount "$${LOOPDEV}p2" mnt2; \
	sudo cp -r image/* mnt1; \
	sudo cp -r filesystem/* mnt1; \
	sudo cp -r filesystem/* mnt2; \
	sudo umount mnt1 mnt2; \
	rm -r mnt1 mnt2; \
	sudo losetup -d "$${LOOPDEV}"

clean:
	rm -f build/main.efi *.o processes/*.o
	rm -rf image
	rm -f build/disk.img
	rm -rf mnt

run:
	qemu-system-x86_64 -bios ./OVMF_X64.fd -no-shutdown -net none -drive file=build/disk.img,if=ide,media=disk -enable-kvm -cpu host -smp 1 -vga virtio -d all -device virtio-gpu,xres=1920,yres=1080 -m 16G -serial mon:stdio