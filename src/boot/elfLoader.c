/* elfLoader.c */
#include <boot/elfLoader.h>
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

typedef struct
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
} __attribute__((packed)) ELFHeader;

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) ELFProgramHeader;

void elfLoader_loadSegment(ELFProgramHeader* ph, ext2_file_t* file_data);

int elfLoader_load(page_table_t* pageTable, shell_t* shell, file_t* file)
{
    (*TEMP)++;
    pageTable_addKernel(pageTable);

    ext2_file_t* file_ext2 = &file->file;

    ext2_file_seek(file_ext2, 0, SEEK_SET);
    ELFHeader header;

    if (ext2_file_read(FILESYSTEM, file_ext2, &header, sizeof(ELFHeader)) != sizeof(ELFHeader))
    {
        /* Failed to read ELF header */
        // stream_write(&FBCON_TTY->user_endpoint, "Failed to read ELF header\n", 0);
        return 1;
    }

    if (header.EI_MAG0 != 0x7F || header.EI_MAG3[0] != 'E' || header.EI_MAG3[1] != 'L' ||
        header.EI_MAG3[2] != 'F')
    {
        /* Not valid elf file */

        // stream_write(&FBCON_TTY->user_endpoint, "Not valid elf file\n", 0);
        return 1;
    }

    if (header.EI_DATA != 1)
    {
        /* Big endian ELF not supported */

        // stream_write(&FBCON_TTY->user_endpoint, "Big endian ELF not supported\n", 0);
        return 1;
    }
    if (header.e_machine != 0x3E)
    {
        /* Architecture not supported */

        // stream_write(&FBCON_TTY->user_endpoint, "Architecture not supported\n", 0);
        return 1;
    }
    if (header.e_type != 2)
    {
        /* not an executable */

        // stream_write(&FBCON_TTY->user_endpoint, "Not an executable\n", 0);
        return 1;
    }

    ext2_file_seek(file_ext2, header.e_phoff, SEEK_SET);

    ELFProgramHeader* phdrs = kmalloc(sizeof(ELFProgramHeader) * header.e_phnum);

    if (ext2_file_read(FILESYSTEM, file_ext2, phdrs, sizeof(ELFProgramHeader) * header.e_phnum) !=
        sizeof(ELFProgramHeader) * header.e_phnum)
    {
        /* failed to read program header(s) */
        return 1;
    }

    uint64_t pid = PROCESS_MEM_FREE_STACK[PROCESS_MEM_FREE_STACK[0]--];

    // if (shell)
    // stream_write(&FBCON_TTY->user_endpoint, "\n", 0);
    for (int i = 0; i < header.e_phnum; i++)
    {
        ELFProgramHeader* ph = &phdrs[i];

        if (ph->p_type == PT_LOAD) /* Loadable Segment */
        {
            /* Seek to start of section */
            ext2_file_seek(file_ext2, ph->p_offset, SEEK_SET);

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
                    long idk = ext2_file_read(FILESYSTEM, file_ext2, page, data_moved);

                    data_left -= MIN(4096, data_left);
                }
                pageTable_addPage(pageTable, ph->p_vaddr + PAGE_SIZE_4KB * i,
                                  (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 4);
                pageTable_addPage(KERNEL_PAGE_TABLE,
                                  (ADDRESS_SECTION_SIZE * (2 + pid)) + ph->p_vaddr +
                                      PAGE_SIZE_4KB * i,
                                  (uint64_t)page / PAGE_SIZE_4KB, 1, PAGE_SIZE_4KB, 0);
            }

            /* Zero out remaining pages */
        }
        else if (ph->p_type == PT_INTERP) /* Dynamic Linker */
        {
            /* Dynamically linked is not supported */
            dev_kernel_fn(VCONS[0].dev_id, DEV_WRITE, "\nWHY AM I HERE\n ", 16);
            return 1;
        }
    }

    void* stackPage = pages_allocatePage(PAGE_SIZE_2MB);

    process_t* process = kaligned_alloc(sizeof(process_t), 16);
    process->page_table = pageTable;
    process->pid = pid;
    process->stackPointer = 0x7FFF00; /* 5mb + 1kb */
    process->entry = header.e_entry;
    process->process_heap_ptr = 0x40000000;     /* 1 gb */
    process->process_shared_ptr = 0x2000000000; /* 128gb */

    process->process_stack_signature.r15 = header.e_entry;
    process->process_stack_signature.r14 = 0x1;
    process->process_stack_signature.r13 = 0x2;
    process->process_stack_signature.r12 = 0x3;
    process->process_stack_signature.r11 = 0x4789;
    process->process_stack_signature.r10 = 0x5;
    process->process_stack_signature.r9 = 0x6;
    process->process_stack_signature.r8 = 0x7;
    process->process_stack_signature.rbp = 0x8;
    process->process_stack_signature.rdi = 0x9;
    process->process_stack_signature.rsi = 0x10;
    process->process_stack_signature.rdx = 0x11;
    process->process_stack_signature.rcx = 0x12;
    process->process_stack_signature.rbx = 0x13;
    process->process_stack_signature.rax = 0x14;
    process->process_stack_signature.rip = header.e_entry;
    process->process_stack_signature.cs = 0x1B; /* kernel - 0x08, user - 0x1B*/
    process->process_stack_signature.rflags = (1 << 9) | (1 << 1);
    process->process_stack_signature.rsp = 0x7FFF00; /* 5mb + 1kb */
    process->process_stack_signature.ss = 0x23;      /* kernel - 0x10, user - 0x23 */
    process->flags = 0;
    process->cwd = file->dir;
    process->heap_end = 0x40000000; /* 1gb */

    pageTable_addPage(pageTable, 0x600000, (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB,
                      4);
    pageTable_addPage(KERNEL_PAGE_TABLE, (ADDRESS_SECTION_SIZE * (2 + pid)) + 0x600000 /* 5mb */,
                      (uint64_t)stackPage / PAGE_SIZE_2MB, 1, PAGE_SIZE_2MB, 0);

    scheduler_schedule(process);
    return 0;
}