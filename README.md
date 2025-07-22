# HendOS

A modern, educational x86_64 operating system kernel and userland, written from scratch in C.

---

## Overview

**HendOS** is a hobbyist operating system designed for learning, experimentation, and exploration of low-level systems programming. It features a modular kernel, custom memory management, multitasking, device drivers, a virtual filesystem, and a small suite of userland programs. The project aims to provide a readable, well-documented codebase for OS enthusiasts and students.

---

## Features

- **x86_64 UEFI Boot**: Boots via UEFI, with custom boot services and ELF loader.
- **Preemptive Multitasking**: Process management, scheduling, and context switching.
- **Virtual Memory**: 4-level paging, custom page allocator, and memory pools.
- **Device Drivers**: Framebuffer console, keyboard, mouse, graphics primitives, EXT2 filesystem.
- **Virtual Filesystem (VFS)**: Unified interface for filesystems and device nodes.
- **Userland Programs**: Shell, file utilities (ls, cp, mv, rm, mkdir, etc.), GUI demo, and more.
- **Custom C Runtime**: Minimal C standard library and process loader.

---

## Requirements

To build and run HendOS, you will need the following tools installed on your system:

- **GCC cross-compiler for x86_64 UEFI (MinGW-w64):**
  - `x86_64-w64-mingw32-gcc`
- **NASM** (Netwide Assembler)
- **GNU Make**
- **QEMU** (for emulation/testing)
- **GNU Parted** (for disk image partitioning)
- **losetup**, **mkfs.fat**, **mkfs.ext2**, **mount**, **umount** (standard Linux utilities for disk image creation and mounting)
- **sudo** (required for some disk image operations)
- **OVMF_X64.fd** (UEFI firmware for QEMU, must be present in the project root)

> **Note:** You may need to install these tools using your system's package manager (e.g., `pacman`, `apt`, `dnf`).

---

## Directory Structure

```
HendOS/
├── crt/           # C runtime startup code
├── filesystem/    # Sample files and fonts for the OS
├── include/       # Kernel and driver headers (modularized)
├── processes/     # Userland programs (shell, ls, cp, etc.)
├── src/           # Kernel source code (arch, drivers, fs, memory, etc.)
├── std/           # Minimal C standard library implementation
├── Makefile       # Top-level build script
└── README.md      # This file
```

- **src/**: Main kernel code, organized by subsystem (arch, boot, drivers, fs, kernel, kstd, memory, std)
- **include/**: Header files for all kernel modules and drivers
- **processes/**: Userland applications, each in its own directory
- **crt/**: C runtime and startup code
- **filesystem/**: Sample files, fonts, and test data
- **std/**: Minimal C standard library (libc subset)

---

## Building and Running

1. **Build the kernel and userland:**
   ```sh
   make
   ```
2. **Run in QEMU:**
   ```sh
   make run
   ```
3. **Clean build artifacts:**
   ```sh
   make clean
   ```

> **Note:** You may need to adjust the Makefile or toolchain paths for your environment. See comments in the Makefile for details.

---

## License

This project is released under the MIT License. See `LICENSE` for details.

---

## Credits & Contact

- Project by [Your Name or Handle]
- Inspired by the OSDev community and educational projects
- For questions or collaboration, open an issue or contact via GitHub

---

> **Note:** This project is for personal/educational use and is not intended for general contributions.
