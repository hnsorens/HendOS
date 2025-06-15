void _start() __attribute__((naked));
void _start()
{
    __asm__ volatile("mov (%rsp), %rdi\n"           // argc → rdi (1st arg)
                     "lea 8(%rsp), %rsi\n"          // argv → rsi (2nd arg)
                     "lea 8(%rsp, %rdi, 8), %rdx\n" // envp → rdx (3rd arg)
                     "xor %rax, %rax\n"             // clear rax (syscall ABI)
                     "call main\n"
                     "mov %rax, %rdi\n" // move main return to rdi (exit status)
                     "mov $1, %rax\n"   // syscall number for exit
                     "int $0x80\n");
}