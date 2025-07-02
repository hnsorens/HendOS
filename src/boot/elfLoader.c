/* elfLoader.c */
#include <boot/elfLoader.h>
#include <fs/fdm.h>
#include <fs/vfs.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kstd/kmath.h>
#include <kstring.h>
#include <memory/kglobals.h>
#include <memory/kmemory.h>
#include <memory/memoryMap.h>
#include <memory/pageTable.h>
#include <memory/paging.h>
#include <misc/debug.h>

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

#define MIN(x, y) (x) < (y) ? (x) : (y)

typedef struct elf_header_t
{
    /* e_ident */
    uint8_t EI_MAG0;
    uint8_t EI_MAG3[3];
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSION;
    uint8_t EI_OSABI;
    uint8_t EI_ABIVERSION;
    uint8_t EI_PAD[7];
    /* info */
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;

    /* 32/64 bit dependent */
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;

    /* continue */
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct elf_program_header_t
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf_program_header_t;

void elfLoader_loadSegment(elf_program_header_t* ph, open_file_t* file_data);

int elfLoader_systemd(open_file_t* file)
{
    page_table_t page_table = 0;
    process_t* process = pool_allocate(*PROCESS_POOL);
    elfLoader_load(&page_table, file, process);
    void* stackPage = pages_allocatePage(PAGE_SIZE_2MB);

    uint64_t pid = process_genPID();

    process->page_table = page_table;
    process->pid = pid;
    process->stackPointer = 0x7FFF00;           /* 5mb + 1kb */
    process->process_heap_ptr = 0x40000000;     /* 1 gb */
    process->process_shared_ptr = 0x2000000000; /* 128gb */

    process->process_stack_signature.r15 = 0;
    process->process_stack_signature.r14 = 0;
    process->process_stack_signature.r13 = 0;
    process->process_stack_signature.r12 = 0;
    process->process_stack_signature.r11 = 0;
    process->process_stack_signature.r10 = 0;
    process->process_stack_signature.r9 = 0;
    process->process_stack_signature.r8 = 0;
    process->process_stack_signature.rbp = 0;
    process->process_stack_signature.rdi = 0;
    process->process_stack_signature.rsi = 0;
    process->process_stack_signature.rdx = 0;
    process->process_stack_signature.rcx = 0;
    process->process_stack_signature.rbx = 0;
    process->process_stack_signature.rax = 0;
    process->process_stack_signature.cs = 0x1B; /* kernel - 0x08, user - 0x1B*/
    process->process_stack_signature.rflags = (1 << 9) | (1 << 1);
    process->process_stack_signature.rsp = 0x7FFF00; /* 5mb + 1kb */
    process->process_stack_signature.ss = 0x23;      /* kernel - 0x10, user - 0x23 */
    process->flags = 0;
    process->cwd = ROOT;
    process->heap_end = 0x40000000; /* 1gb */
    process->file_descriptor_capacity = 4;
    process->file_descriptor_count = 0;
    process->waiting_parent_pid = 0;
    process->file_descriptor_table = kmalloc(sizeof(file_descriptor_t*) * process->file_descriptor_capacity);
    process->signal = SIGNONE;

    pageTable_addPage(&process->page_table, 0x600000, (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 4);

    /* Configure arguments */
    void* args_page = pages_allocatePage(PAGE_SIZE_2MB);
    pageTable_addPage(&process->page_table, 0x200000, (uint64_t)args_page / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 4);
    scheduler_schedule(process);
    return 0;
}

int elfLoader_load(page_table_t* page_table_ptr, open_file_t* open_file, process_t* process)
{
    pageTable_addKernel(page_table_ptr);

    ext2_file_seek(open_file, 0, SEEK_SET);
    elf_header_t header;

    if (ext2_file_read(FILESYSTEM, open_file, &header, sizeof(elf_header_t)) != sizeof(elf_header_t))
    {
        /* Failed to read ELF header */
        return 1;
    }

    if (header.EI_MAG0 != 0x7F || header.EI_MAG3[0] != 'E' || header.EI_MAG3[1] != 'L' || header.EI_MAG3[2] != 'F')
    {
        /* Not valid elf file */
        return 1;
    }

    if (header.EI_DATA != 1)
    {
        /* Big endian ELF not supported */
        return 1;
    }
    if (header.e_machine != 0x3E)
    {
        /* Architecture not supported */
        return 1;
    }
    if (header.e_type != 2)
    {
        /* not an executable */
        return 1;
    }

    ext2_file_seek(open_file, header.e_phoff, SEEK_SET);

    elf_program_header_t* phdrs = kmalloc(sizeof(elf_program_header_t) * header.e_phnum);

    if (ext2_file_read(FILESYSTEM, open_file, phdrs, sizeof(elf_program_header_t) * header.e_phnum) != sizeof(elf_program_header_t) * header.e_phnum)
    {
        /* failed to read program header(s) */
        return 1;
    }

    for (int i = 0; i < header.e_phnum; i++)
    {
        elf_program_header_t* ph = &phdrs[i];

        if (ph->p_type == PT_LOAD) /* Loadable Segment */
        {
            /* Seek to start of section */
            ext2_file_seek(open_file, ph->p_offset, SEEK_SET);

            /* find size of section in pages */
            uint64_t virtual_mem_end = ALIGN_UP(ph->p_vaddr + ph->p_memsz, 4096);
            uint64_t virtual_mem_start = ALIGN_DOWN(ph->p_vaddr, 4096);
            uint64_t page_count = (virtual_mem_end - virtual_mem_start) / 4096;

            int data_left = ph->p_filesz;
            for (uint64_t i = 0; i < page_count; i++)
            {
                void* page = pages_allocatePage(PAGE_SIZE_4KB);
                kmemset(page, 0, 4096);
                if (data_left > 0)
                {
                    int data_moved = MIN(4096, data_left);
                    long idk = ext2_file_read(FILESYSTEM, open_file, page, data_moved);

                    data_left -= MIN(4096, data_left);
                }
                pageTable_addPage(page_table_ptr, ph->p_vaddr + PAGE_SIZE_4KB * i, (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 4);
            }

            /* Zero out remaining pages */
        }
        else if (ph->p_type == PT_INTERP) /* Dynamic Linker */
        {
            /* Dynamically linked is not supported */
            return 1;
        }
    }

    process->entry = header.e_entry;
    process->process_stack_signature.rip = header.e_entry;
}