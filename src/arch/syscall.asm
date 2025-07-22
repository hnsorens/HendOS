;**
; @file syscall.asm
; @brief x86_64 Syscall Entry Stub (commented reference)
;
; Contains (commented) reference code for syscall entry/exit handling.
;**
; bits 64
; section .text

; global syscall_stub
; syscall_stub:

;     ; mov r10, rsp
;     ; mov rsp, 0x37FFFFFF00 ; TODO - get these actual values somehow
;     ; push r10

;     ; push rcx               ; must save: holds return address
;     ; push r11               ; must save: holds return flags
    
;     ; push rsi
;     ; push rdi
;     ; push rbp
;     ; push r8
;     ; push r9
;     ; push r10
;     ; push r11
;     ; push r12
;     ; push r13
;     ; push r14
;     ; push r15
    
;     ; mov r12, cr3
;     ; push r12
    
;     ; mov r15, 0x1000
;     ; mov cr3, r15

;     ; mov rcx, 0x00000037b9dbcd88
;     ; mov rbx, rax

;     ; shl rbx, 3
;     ; add rcx, rbx

;     ; mov rax, [rcx]

;     ; call rax

;     ; pop r12
;     ; mov cr3, r12

;     ; pop r15
;     ; pop r14
;     ; pop r13
;     ; pop r12
;     ; pop r11
;     ; pop r10
;     ; pop r9
;     ; pop r8
;     ; pop rbp
;     ; pop rdi
;     ; pop rsi

;     ; pop r11
;     ; pop rcx

;     ; pop r10
;     ; mov rsp, r10
; hlt
;     sysretq
