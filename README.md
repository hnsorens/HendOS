# ![Project Logo/Banner](path/to/logo_or_banner.png)

# HendOS

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://shields.io) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE) [![Platform](https://img.shields.io/badge/platform-x86__64-blue)](https://shields.io)

---

## Table of Contents
- [Demo](#demo)
- [Why I Built This](#why-i-built-this)
- [Learning Outcomes](#learning-outcomes)
- [Features](#features)
- [Directory Structure](#directory-structure)
- [Building and Running](#building-and-running)
- [How to Contribute](#how-to-contribute)
- [License](#license)
- [Credits & Contact](#credits--contact)

---

## Why I Built This

I created HendOS as a way to deeply understand how operating systems work from the ground up. My goal was to learn about low-level systems programming, memory management, multitasking, and hardware interaction by building a real, working kernel and userland. This project is both a personal challenge and a resource for others interested in OS development.

---

## Learning Outcomes

- Gained hands-on experience with x86_64 architecture and UEFI booting
- Learned about memory management, paging, and virtual memory
- Implemented multitasking, process scheduling, and system calls
- Built device drivers for graphics, keyboard, mouse, and filesystems
- Designed and implemented a virtual filesystem and minimal libc
- Improved my C programming, debugging, and documentation skills

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

> **For a more in-depth overview of features and technical details, see the full documentation at:**
> [https://hnsorens.github.io/docs/hend-os.html](https://hnsorens.github.io/docs/hend-os.html)

---

## Requirements

To build and run HendOS, you will need the following tools installed on your system:

- **GCC cross-compiler for x86_64 UEFI (MinGW-w64):**
  - `x86_64-w64-mingw32-gcc`
- **NASM** (Netwide Assembler)
- **GNU Make**
- **QEMU** (for emulation/testing)
- **GNU Parted** (for disk image partitioning)
- **losetup** (loop device setup)
- **mkfs.fat** (FAT filesystem creation)
- **mkfs.ext2** (EXT2 filesystem creation)
- **mount** and **umount** (mounting/unmounting filesystems)
- **sudo** (required for some disk image operations)
- **OVMF_X64.fd** (UEFI firmware for QEMU, must be present in the project root)
- **dd** (for creating disk images)
- **cp**, **rm**, **find**, **mkdir** (standard Unix utilities)

> **On Arch Linux:**
> - Install QEMU and all utilities with:
>   ```sh
>   sudo pacman -S qemu-all
>   ```
> - Other tools: `sudo pacman -S gcc nasm make parted dosfstools e2fsprogs`
>
> **On Debian/Ubuntu:**
> - Install QEMU and utilities with:
>   ```sh
>   sudo apt install qemu-system-x86 qemu-utils ovmf
>   ```
> - Other tools: `sudo apt install gcc nasm make parted dosfstools e2fsprogs`

### Building gnu-efi from Submodules
If you are building `gnu-efi` from source (as a submodule), use:
```sh
make ARCH=x86_64 CROSS_COMPILE=x86_64-w64-mingw32-
```
This will build the x86_64 UEFI libraries and objects needed for this project.

---

## Directory Structure

```
HendOS/
├── crt/           # C runtime startup code
├── filesystem/    # Sample files and fonts for the OS
├── include/       # Kernel and driver headers (modularized)
├── processes/     # Userland programs (shell, ls, cp, etc.)
├── tests/         # Userland tests
├── src/           # Kernel source code (arch, drivers, fs, memory, etc.)
├── std/           # Minimal C standard library implementation
├── Makefile       # Top-level build script
└── README.md      # This file
```

- **src/**: Main kernel code, organized by subsystem (arch, boot, drivers, fs, kernel, kstd, memory, std)
- **include/**: Header files for all kernel modules and drivers
- **processes/**: Userland applications, each in its own directory
- **tests/**: Userland test files, each c file is a test
- **crt/**: C runtime and startup code
- **filesystem/**: Sample files, fonts, and test data
- **std/**: Minimal C standard library (libc subset)

---

## Building and Running

1. **Build operating system**
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

---

## How to Contribute

This project is primarily for personal and educational use, but I welcome feedback, bug reports, and suggestions! If you’d like to contribute code, please open an issue or pull request to discuss your ideas first. See the LICENSE file for details on usage and contributions.

---

## License

This project is released under the MIT License. See `LICENSE` for details.

---

## Credits & Contact

- Inspired by the OSDev community and educational projects
- For questions or collaboration, open an issue or contact via GitHub

---

> **Note:** This project is for personal/educational use and is not intended for general contributions.