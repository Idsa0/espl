#include <elf.h>
#include <stdio.h>
// #include <stdlib.h> // TODO maybe remove
#include <sys/mman.h>

extern int system_call();
extern int startup(int argc, char **argv, void (*start)());

#define exit(status) system_call(1, (status), 0, 0)

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

// #ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
// #endif

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg);
void print_phdr(Elf32_Phdr *phdr, int arg);
static inline void pprint_phdrs(void *map_start);
void load_phdr(Elf32_Phdr *phdr, int fd);
int startup(int argc, char **argv, void (*start)());
void print_mmap_prot_flags(Elf32_Phdr *phdr, int arg);

size_t filesize = 0;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (!file)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);

    // load the file into memory using mmap
    void *map_start = mmap(NULL, filesize, PROT_READ | PROT_EXEC | PROT_WRITE, MAP_PRIVATE, fileno(file), 0);

    foreach_phdr(map_start, load_phdr, fileno(file));
    fclose(file);
    pprint_phdrs(map_start);
    startup(argc - 1, argv + 1, (void *)((Elf32_Ehdr *)map_start)->e_entry);

    munmap(map_start, filesize);

    return 0;
}

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i)
        func(phdr++, arg);

    return 0;
}

static inline char *get_phdr_type(const Elf32_Phdr *phdr)
{
    switch (phdr->p_type)
    {
    case PT_NULL:
        return "NULL";
    case PT_LOAD:
        return "LOAD";
    case PT_DYNAMIC:
        return "DYNAMIC";
    case PT_INTERP:
        return "INTERP";
    case PT_NOTE:
        return "NOTE";
    case PT_SHLIB:
        return "SHLIB";
    case PT_PHDR:
        return "PHDR";
    case PT_TLS:
        return "TLS";
    case PT_GNU_EH_FRAME:
        return "GNU_EH_FRAME";
    case PT_GNU_STACK:
        return "GNU_STACK";
    case PT_GNU_RELRO:
        return "GNU_RELRO";
    default:
        return "UNKNOWN";
    }
}

void print_phdr(Elf32_Phdr *phdr, int arg)
{
    printf("%-12s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %c%c%c 0x%X\n",
           get_phdr_type(phdr),
           phdr->p_offset,
           phdr->p_vaddr,
           phdr->p_paddr,
           phdr->p_filesz,
           phdr->p_memsz,
           phdr->p_flags & PF_R ? 'R' : ' ',
           phdr->p_flags & PF_W ? 'W' : ' ',
           phdr->p_flags & PF_X ? 'X' : ' ',
           phdr->p_align);
}

static inline void pprint_phdrs(void *map_start)
{
    printf("Type         Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
    foreach_phdr(map_start, print_phdr, 0);
}

void print_mmap_prot_flags(Elf32_Phdr *phdr, int arg)
{
    int prot = 0;
    if (phdr->p_flags & PF_R)
        prot |= PROT_READ;
    if (phdr->p_flags & PF_W)
        prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
        prot |= PROT_EXEC;

    printf("0x%x\n", prot);
}

void load_phdr(Elf32_Phdr *phdr, int fd)
{
    if (phdr == MAP_FAILED)
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    // check if the file is an elf file
    if (phdr->p_type != PT_LOAD)
        return;

    // check prot
    int protFlags = 0;
    if (phdr->p_flags & PF_R)
        protFlags |= PROT_READ;
    if (phdr->p_flags & PF_W)
        protFlags |= PROT_WRITE;
    if (phdr->p_flags & PF_X)
        protFlags |= PROT_EXEC;

    // align
    void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
    unsigned int offset = phdr->p_offset & 0xfffff000;
    unsigned int padding = phdr->p_vaddr & 0xfff;

    if (mmap(vaddr, phdr->p_memsz + padding, protFlags, MAP_PRIVATE | MAP_FIXED, fd, offset) == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
}
