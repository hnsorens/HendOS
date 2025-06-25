void _start() __attribute__((naked));
void _start()
{
    __asm__ volatile("mov (%%rsp), %%rdi\n\t"            // argc → rdi (1st arg)
                     "lea 8(%%rsp), %%rsi\n\t"           // argv → rsi (2nd arg)
                     "lea 8(%%rsp, %%rdi, 8), %%rdx\n\t" // envp → rdx (3rd arg)
                     "xor %%rax, %%rax\n\t"              // clear rax (syscall ABI)
                     "call main\n\t"
                     "mov %%rax, %%rdi\n\t" // move main return to rdi (exit status)
                     "mov $1, %%rax\n\t"    // syscall number for exit
                     "int $0x80\n\t"
                     :
                     :
                     : "rax", "rdi");
}