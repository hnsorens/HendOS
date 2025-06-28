bits 64
section .text
extern exception_handler
extern interrupt_handler

; Macro for exceptions WITHOUT error code
%macro isr_no_err 1
isr_stub_%+%1:

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
    mov rsp, 0xFFFF810000FFFF00 ; TODO - get these actual values somehow
    push r14
    mov r12, cr3
    push r12
    push 0
    push %1

    ; Set irq number and run exception handler
    call interrupt_handler

    ; mov back to original stack and page table
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    mov al, 0x20
    out 0x20, al  ; Master PIC EOI

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

    iretq                ; Return from interrupt (64-bit)

%endmacro



; Macro for exceptions WITH error code
%macro isr_err 1
isr_stub_%+%1:
    pop r15 ; Error code
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
    mov rsp, 0xFFFF810000FFFF00 ; TODO - get these actual values somehow
    push r14
    mov r12, cr3
    push r12
    push r15
    push %1

    ; Set irq number and run exception handler
    call exception_handler

    ; mov back to original stack and page table
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    mov al, 0x20
    out 0x20, al  ; Master PIC EOI

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
    iretq                ; Return from interrupt (64-bit)
%endmacro

; Generate all exception stubs (0-31)
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

syscall_stub:

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
    mov rsp, 0xFFFF810000FFFF00 ; TODO - get these actual values somehow
    push r14
    mov r12, cr3
    push r12
    push 0
    push 0x80
    
    mov rcx, 0xffff8600001a4c00
    mov rbx, rax

    shl rbx, 3
    add rcx, rbx

    mov rax, [rcx]

    call rax

    ; mov back to original stack and page table
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp

    mov al, 0x20
    out 0x20, al  ; Master PIC EOI

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


; Macro for Tick
isr_stub_32:
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
    mov rsp, 0xFFFF810000FFFF00 ; TODO - get these actual values somehow
    push r14
    mov r12, cr3
    push r12
    push 0
    push 32

    ; Set irq number and run exception handler
    call interrupt_handler
    ; mov back to original stack and page table
    pop r13
    pop r13
    pop r13
    mov cr3, r13
    pop rsp


    mov al, 0x20
    out 0x20, al  ; Master PIC EOI

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
    iretq                ; Return from interrupt (64-bit)

; Generate stubs for remaining interrupts (33-255) (32 is tick and is defined above)
%assign i 33
%rep (256 - 33)
    isr_no_err i
%assign i i+1
%endrep

; Global ISR table (256 entries)
global isr_stub_table
isr_stub_table:
dq isr_stub_0
dq isr_stub_1
dq isr_stub_2
dq isr_stub_3
dq isr_stub_4
dq isr_stub_5
dq isr_stub_6
dq isr_stub_7
dq isr_stub_8
dq isr_stub_9
dq isr_stub_10
dq isr_stub_11
dq isr_stub_12
dq isr_stub_13
dq isr_stub_14
dq isr_stub_15
dq isr_stub_16
dq isr_stub_17
dq isr_stub_18
dq isr_stub_19
dq isr_stub_20
dq isr_stub_21
dq isr_stub_22
dq isr_stub_23
dq isr_stub_24
dq isr_stub_25
dq isr_stub_26
dq isr_stub_27
dq isr_stub_28
dq isr_stub_29
dq isr_stub_30
dq isr_stub_31
dq isr_stub_32
dq isr_stub_33
dq isr_stub_34
dq isr_stub_35
dq isr_stub_36
dq isr_stub_37
dq isr_stub_38
dq isr_stub_39
dq isr_stub_40
dq isr_stub_41
dq isr_stub_42
dq isr_stub_43
dq isr_stub_44
dq isr_stub_45
dq isr_stub_46
dq isr_stub_47
dq isr_stub_48
dq isr_stub_49
dq isr_stub_50
dq isr_stub_51
dq isr_stub_52
dq isr_stub_53
dq isr_stub_54
dq isr_stub_55
dq isr_stub_56
dq isr_stub_57
dq isr_stub_58
dq isr_stub_59
dq isr_stub_60
dq isr_stub_61
dq isr_stub_62
dq isr_stub_63
dq isr_stub_64
dq isr_stub_65
dq isr_stub_66
dq isr_stub_67
dq isr_stub_68
dq isr_stub_69
dq isr_stub_70
dq isr_stub_71
dq isr_stub_72
dq isr_stub_73
dq isr_stub_74
dq isr_stub_75
dq isr_stub_76
dq isr_stub_77
dq isr_stub_78
dq isr_stub_79
dq isr_stub_80
dq isr_stub_81
dq isr_stub_82
dq isr_stub_83
dq isr_stub_84
dq isr_stub_85
dq isr_stub_86
dq isr_stub_87
dq isr_stub_88
dq isr_stub_89
dq isr_stub_90
dq isr_stub_91
dq isr_stub_92
dq isr_stub_93
dq isr_stub_94
dq isr_stub_95
dq isr_stub_96
dq isr_stub_97
dq isr_stub_98
dq isr_stub_99
dq isr_stub_100
dq isr_stub_101
dq isr_stub_102
dq isr_stub_103
dq isr_stub_104
dq isr_stub_105
dq isr_stub_106
dq isr_stub_107
dq isr_stub_108
dq isr_stub_109
dq isr_stub_110
dq isr_stub_111
dq isr_stub_112
dq isr_stub_113
dq isr_stub_114
dq isr_stub_115
dq isr_stub_116
dq isr_stub_117
dq isr_stub_118
dq isr_stub_119
dq isr_stub_120
dq isr_stub_121
dq isr_stub_122
dq isr_stub_123
dq isr_stub_124
dq isr_stub_125
dq isr_stub_126
dq isr_stub_127
dq syscall_stub
dq isr_stub_129
dq isr_stub_130
dq isr_stub_131
dq isr_stub_132
dq isr_stub_133
dq isr_stub_134
dq isr_stub_135
dq isr_stub_136
dq isr_stub_137
dq isr_stub_138
dq isr_stub_139
dq isr_stub_140
dq isr_stub_141
dq isr_stub_142
dq isr_stub_143
dq isr_stub_144
dq isr_stub_145
dq isr_stub_146
dq isr_stub_147
dq isr_stub_148
dq isr_stub_149
dq isr_stub_150
dq isr_stub_151
dq isr_stub_152
dq isr_stub_153
dq isr_stub_154
dq isr_stub_155
dq isr_stub_156
dq isr_stub_157
dq isr_stub_158
dq isr_stub_159
dq isr_stub_160
dq isr_stub_161
dq isr_stub_162
dq isr_stub_163
dq isr_stub_164
dq isr_stub_165
dq isr_stub_166
dq isr_stub_167
dq isr_stub_168
dq isr_stub_169
dq isr_stub_170
dq isr_stub_171
dq isr_stub_172
dq isr_stub_173
dq isr_stub_174
dq isr_stub_175
dq isr_stub_176
dq isr_stub_177
dq isr_stub_178
dq isr_stub_179
dq isr_stub_180
dq isr_stub_181
dq isr_stub_182
dq isr_stub_183
dq isr_stub_184
dq isr_stub_185
dq isr_stub_186
dq isr_stub_187
dq isr_stub_188
dq isr_stub_189
dq isr_stub_190
dq isr_stub_191
dq isr_stub_192
dq isr_stub_193
dq isr_stub_194
dq isr_stub_195
dq isr_stub_196
dq isr_stub_197
dq isr_stub_198
dq isr_stub_199
dq isr_stub_200
dq isr_stub_201
dq isr_stub_202
dq isr_stub_203
dq isr_stub_204
dq isr_stub_205
dq isr_stub_206
dq isr_stub_207
dq isr_stub_208
dq isr_stub_209
dq isr_stub_210
dq isr_stub_211
dq isr_stub_212
dq isr_stub_213
dq isr_stub_214
dq isr_stub_215
dq isr_stub_216
dq isr_stub_217
dq isr_stub_218
dq isr_stub_219
dq isr_stub_220
dq isr_stub_221
dq isr_stub_222
dq isr_stub_223
dq isr_stub_224
dq isr_stub_225
dq isr_stub_226
dq isr_stub_227
dq isr_stub_228
dq isr_stub_229
dq isr_stub_230
dq isr_stub_231
dq isr_stub_232
dq isr_stub_233
dq isr_stub_234
dq isr_stub_235
dq isr_stub_236
dq isr_stub_237
dq isr_stub_238
dq isr_stub_239
dq isr_stub_240
dq isr_stub_241
dq isr_stub_242
dq isr_stub_243
dq isr_stub_244
dq isr_stub_245
dq isr_stub_246
dq isr_stub_247
dq isr_stub_248
dq isr_stub_249
dq isr_stub_250
dq isr_stub_251
dq isr_stub_252
dq isr_stub_253
dq isr_stub_254
dq isr_stub_255



extern gdt_flush


gdt_flush:
    mov rax, rdi
    lgdt [rax]
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    push qword 0x08
    lea rax, [rel .flush]
    push rax
    retfq
.flush:
    ret


extern end_of_function

end_of_function:
    jmp long_mode_entry  ; 64-bit near jump to the entry point
long_mode_entry:
    ret  ; Return from the function