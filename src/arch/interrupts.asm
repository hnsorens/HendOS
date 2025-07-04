bits 64
section .text

; External handler functions that will be called from our ISRs
extern idt_exception_handler   ; Handles CPU exceptions
extern idt_interrupt_handler  ; Handles hardware interrupts
extern check_signal       ; Checks for pending signals

; Macro for ISRs that don't push an error code
; %1 = interrupt number
%macro isr_no_err 1
isr_stub_%+%1:
    ; Save all general purpose registers to preserve context
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

    ; Switch to kernel stack (0xFFFF810000FFFF00 is a placeholder)
    mov r14, rsp                  ; Save current stack pointer
    mov rsp, 0xFFFF810000FFFF00   ; Load kernel stack pointer
    push r14                      ; Save original stack pointer
    mov r12, cr3                  ; Save current page table
    push r12
    push 0                        ; Push dummy error code (0)
    push %1                       ; Push interrupt number

    ; Call appropriate handlers
    call idt_interrupt_handler        ; Handle the interrupt
    call check_signal             ; Check for pending signals

    ; Restore context
    pop r13                       ; Clean up interrupt number
    pop r13                       ; Clean up error code
    pop r13                       ; Get saved CR3
    mov cr3, r13                  ; Restore page table
    pop rsp                       ; Restore original stack pointer

    ; Send EOI (End Of Interrupt) to PIC
    mov al, 0x20
    out 0x20, al

    ; Restore all general purpose registers
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

    iretq                         ; Return from interrupt
%endmacro

; Macro for ISRs that push an error code
; %1 = interrupt number
%macro isr_err 1
isr_stub_%+%1:
    pop r15                       ; Pop error code into r15
    ; Save all registers (same as isr_no_err)
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
    push r15                      ; Push real error code (from r15)
    push %1                       ; Push interrupt number

    ; Call exception handler instead of interrupt handler
    call idt_exception_handler
    call check_signal

    ; Restore context (same as isr_no_err)
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al

    ; Restore registers
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

    iretq
%endmacro

; ==============================================
; Exception handlers (0-31)
; ==============================================
isr_no_err 0    ; Divide-by-zero
isr_no_err 1    ; Debug
isr_no_err 2    ; NMI
isr_no_err 3    ; Breakpoint
isr_no_err 4    ; Overflow
isr_no_err 5    ; Bound Range Exceeded
isr_no_err 6    ; Invalid Opcode
isr_no_err 7    ; Device Not Available
isr_err   8     ; Double Fault
isr_no_err 9    ; Coprocessor Segment Overrun
isr_err   10    ; Invalid TSS
isr_err   11    ; Segment Not Present
isr_err   12    ; Stack-Segment Fault
isr_err   13    ; General Protection Fault
isr_err   14    ; Page Fault
isr_no_err 15   ; Reserved
isr_no_err 16   ; x87 Floating-Point Exception
isr_err   17    ; Alignment Check
isr_no_err 18   ; Machine Check
isr_no_err 19   ; SIMD Floating-Point Exception
isr_no_err 20   ; Virtualization Exception
isr_no_err 21   ; Reserved
isr_no_err 22   ; Reserved
isr_no_err 23   ; Reserved
isr_no_err 24   ; Reserved
isr_no_err 25   ; Reserved
isr_no_err 26   ; Reserved
isr_no_err 27   ; Reserved
isr_no_err 28   ; Reserved
isr_no_err 29   ; Reserved
isr_err   30    ; Security Exception
isr_no_err 31   ; Reserved

; ==============================================
; System call handler (128)
; ==============================================
syscall_stub:
    ; Save all registers
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
    push 0x80             ; System call interrupt number (128)

    ; System call dispatch:
    ; rax contains system call number
    ; rcx points to system call table (0xffff8600001a4ba8)
    mov rcx, 0xffff8600001a4ba8  ; System call table base
    mov rbx, rax                 ; System call number
    shl rbx, 3                   ; Multiply by 8 (each entry is 8 bytes)
    add rcx, rbx                 ; Calculate address of handler
    mov rax, [rcx]               ; Load handler address

    call rax                     ; Call the system call handler
    call check_signal            ; Check for signals

    ; Restore context
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al

    ; Restore registers
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

    iretq

; ==============================================
; IRQ 0 handler (32)
; ==============================================
isr_stub_32:
    ; Same structure as other ISRs
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

    mov r14, rsp
    mov rsp, 0xFFFF810000FFFF00
    push r14
    mov r12, cr3
    push r12
    push 0
    push 32

    call idt_interrupt_handler
    call check_signal

    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    mov al, 0x20
    out 0x20, al

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
    iretq

; ==============================================
; Generate remaining ISRs (33-127, 129-255)
; ==============================================
%assign i 33
%rep (256 - 33)
    %if i != 128  ; Skip 128 (syscall)
        isr_no_err i
    %endif
    %assign i i+1
%endrep

; ==============================================
; Global ISR table (256 entries)
; ==============================================
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    %if i == 128
        dq syscall_stub  ; Special entry for syscall
    %else
        dq isr_stub_%+i  ; All other entries
    %endif
    %assign i i+1
%endrep

; ==============================================
; GDT flush function
; ==============================================
extern gdt_flush
gdt_flush:
    mov rax, rdi        ; Load GDT pointer from first argument
    lgdt [rax]          ; Load new GDT

    ; Load segment registers with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS with kernel code selector (0x08)
    push qword 0x08
    lea rax, [rel .flush]
    push rax
    retfq
.flush:
    ret

; ==============================================
; End of function marker
; ==============================================
extern end_of_function
end_of_function:
    jmp long_mode_entry  ; Jump to 64-bit entry point
long_mode_entry:
    ret                  ; Return