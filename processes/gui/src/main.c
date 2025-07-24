

int main()
{
    unsigned int* framebuffer = (unsigned int*)0x2000000000;
    for (int i = 0; i < 1080 * 1920; i++)
    {
        framebuffer[i] = 0xFF00FFFF;
    }

    __asm__ volatile("mov $1, %%rax\n\t"
                     "mov $0, %%rbx\n\t"
                     "int $0x80\n\t"
                     :
                     :
                     : "rax", "rbx");
}