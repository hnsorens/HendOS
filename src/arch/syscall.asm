bits 64

global syscall_stub
extern check_signal

section .text
syscall_stub:
hlt
    ; Save volatile registers (caller-saved)
    mov r15, rsp
    mov r11, 0xffff8600001ffe24  ; Load address into rax
    mov r11, [r11]               ; Dereference it\
    mov rsp, r11
    mov rsp, r11
    push 0x23
    push r15
    push r11
    push 0x1b
    push rcx

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Switch to kernel stack
    mov r14, rsp
    mov rsp, 0xFFFF810000FFFF00
    push r14
    mov r12, cr3
    push r12
    push 0                ; Dummy error code
    push 0x80             ; Syscall return rsp

    ; Optionally swap to kernel stack (if needed manually)
    ; If you're using per-process kernel stacks, do it here
    ; For now assume we're okay staying on current stack

    ; System call dispatch:
    ; rax: syscall number
    ; Dispatch directly from table (just like you did before)
    mov rcx, 0xffff8600001a4ba8  ; address of syscall table
    mov rbx, rax
    shl rbx, 3
    add rcx, rbx
    mov rax, [rcx]
    call rax

    ; Optional signal check
    call check_signal

    ; Restore context
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp


    ; Restore volatile registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rcx
    add rsp, 8
    pop r11
    pop rsp

    sysret