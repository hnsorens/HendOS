# 🧵 OS Development Checklist

---

## 🪛 Boot & Kernel Init
- [X] UEFI bootloader or boot stub
- [X] Assembly startup → `kernel_main()`
- [X] Page-aligned kernel memory layout
- [ ] Serial or framebuffer output
- [ ] Basic `kprintf()` or `printf()` for kernel

---

## 🧠 Memory Management
- [X] Parse physical memory map (from UEFI/BIOS)
- [X] Physical memory allocator (bitmap/buddy)
- [X] Page table setup (PML4 on x86_64)
- [X] Virtual memory mapping
- [X] Page fault handling
- [X] Kernel heap allocator (`kmalloc`, `kfree`)

---

## ⚙️ Interrupt Handling
- [X] IDT (Interrupt Descriptor Table)
- [X] ISRs for CPU faults (e.g., page fault)
- [X] APIC or PIC initialization
- [X] Timer (PIT or HPET) setup
- [X] Timer interrupts
- [X] Context switch code

---

## 🧵 Process Management
- [X] Process Control Block (PCB) design
- [X] `fork()` system call
- [X] ELF loader
- [X] User/kernel mode transitions
- [X] Scheduler (round-robin or priority)
- [X] `execve()`, `exit()`, `waitpid()`
- [X] Zombie reaping logic

---

## 👪 Process Groups & Signals
- [X] Process Group ID (PGID) support
- [X] Session ID (SID) and `setsid()`
- [X] `setpgid()`, `getpgid()` syscalls
- [X] Signal definitions (SIGKILL, SIGINT, etc.)
- [O] Signal delivery engine
- [O] Signal masking per process
- [O] `kill()`, `sigaction()`, `sigreturn()`

---

## 💬 Terminals & Shell Support
- [X] Keyboard driver
- [X] Line discipline (echo, backspace, etc.)
- [X] TTY master/slave (PTY)
- [ ] Controlling terminal support
- [X] `tcgetpgrp()`, `tcsetpgrp()`
- [X] Foreground/background job control
- [X] Userland shell

---

## 📄 File & I/O System
- [X] File descriptor table per process
- [ ] `read()`, `write()`, `close()`, `dup2()`
- [X] VFS abstraction
- [X] ext2 driver
- [ ] Mount system (`mount()`, `umount()`)
- [ ] Pipes (`pipe()`)

---

## 🔒 Permissions & Users
- [ ] Per-process UID / GID tracking
- [ ] File permission bits (`rwx`)
- [ ] Syscalls: `getuid()`, `setuid()`, etc.
- [ ] `chmod()`, `chown()`, `access()`
- [ ] Access control on syscalls and FS

---

## 🔧 Syscalls & libc
- [X] Syscall mechanism (`syscall` or `int 0x80`)
- [X] Syscall table and dispatcher
- [X] Minimal `libc` implementation:
  - `malloc`, `free`, `printf`, `str*`
  - `open`, `close`, `read`, `write`
  - `fork`, `exec`, `wait`, `exit`

---

## 📦 Init System & Services
- [X] PID 1: `init` process
- [X] Start shell from `init`
- [ ] `syslogd` or log sink
- [ ] Daemon support (detach & fork)
- [ ] Optional services: cron, udev, network
- [ ] Shutdown and reboot signals

---

## 🛠 Tooling
- [X] Cross-compiler toolchain
- [X] QEMU run script
- [X] Disk image builder
