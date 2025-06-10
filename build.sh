

ASM_DIR="processes"
OUTPUT_DIR="filesystem"


# Loop through each .asm file in the processes/ directory
for asm_file in "$ASM_DIR"/*.asm; do
    # Get the base filename without path or extension
    base=$(basename "$asm_file" .asm)
    obj_file="$ASM_DIR/$base.o"
    bin_file="$OUTPUT_DIR/$base"

    echo "🔧 Assembling $asm_file → $obj_file"
    nasm -f elf64 "$asm_file" -o "$obj_file"

    echo "🔗 Linking $obj_file → $bin_file"
    ld -static -o "$bin_file" "$obj_file"

    echo "✅ Built $bin_file"
done

rm -f build/main.efi *.o
rm -rf image
rm build/disk.img
rm mnt

# find src include \( -name '*.c' -o -name '*.h' \) | xargs clang-format -i -style=file
# clang-tidy src/*.c -- -Iinclude -std=gnu99

mkdir -p build
x86_64-w64-mingw32-gcc -fPIC -pie -m64 -mno-red-zone -fno-stack-protector -Wall -Wextra -nostdlib -Ignu-efi/inc -Ignu-efi/inc/x86_64 -Ignu-efi/inc/protocol -DCONFIG_x86_64 -D__MAKEWITH_GNUEFI -DGNU_EFI_USE_MS_ABI -D_DEBUG -ffreestanding -c -Iinclude/kstd -Iinclude/std -Iinclude $(find src -name '*.c')
nasm -f elf64 src/arch/interrupts.asm -o interrupts.o
x86_64-w64-mingw32-gcc -Wl,--image-base=0x00000FFF00000000 -fPIC -pie -Wl,-dll -Wl,--subsystem,10 -Lgnu-efi/x86_64/lib -e efi_main -s -Wl,-Bsymbolic -nostdlib -shared $(find src -name '*.c' | sed 's|.*/||; s/\.c$/.o/') interrupts.o -o build/main.efi -lefi
find "$(dirname "$0")" -maxdepth 2 -type f -name '*.o' -delete
mkdir -p image/efi/boot
cp -f build/main.efi image/efi/boot/bootx64.efi

#!/bin/bash
set -e

# Create disk image
dd if=/dev/zero of=build/disk.img bs=1M count=128

# Partition the image
sudo parted build/disk.img --script mklabel gpt
sudo parted build/disk.img --script mkpart primary fat32 1MiB 64MiB
sudo parted build/disk.img --script mkpart primary 64MiB 100%

# Setup loop device
LOOPDEV=$(sudo losetup --find --partscan --show build/disk.img)

# Format partitions
sudo mkfs.fat -F32 "${LOOPDEV}p1"
sudo mkfs.ext2 "${LOOPDEV}p2"

# Mount and copy files
mkdir -p mnt1 mnt2
sudo mount "${LOOPDEV}p1" mnt1
sudo mount "${LOOPDEV}p2" mnt2

sudo cp -r image/* mnt1
sudo cp -r filesystem/* mnt1
sudo cp -r filesystem/* mnt2

# Clean up
sudo umount mnt1 mnt2
rm -r mnt1 mnt2
sudo losetup -d "${LOOPDEV}"


qemu-system-x86_64  -bios ./OVMF_X64.fd -no-shutdown -net none -drive file=build/disk.img,if=ide,media=disk -enable-kvm -cpu host  -smp 1 -vga virtio -d all -device virtio-gpu,xres=1920,yres=1080 -m 16G -serial mon:stdio
