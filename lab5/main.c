#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg);
void print_phdr(const Elf32_Phdr *phdr, int arg);

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
    size_t size = ftell(file);
    rewind(file);

    void *map_start = malloc(size);
    if (!map_start)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (fread(map_start, 1, size, file) != size)
    {
        perror("fread");
        exit(EXIT_FAILURE);
    }

    fclose(file);
    printf("Type         Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
    foreach_phdr(map_start, (void (*)(Elf32_Phdr *, int))print_phdr, 0);
    free(map_start);

    return 0;
}

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(map_start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i)
    {
        func(phdr, arg);
        ++phdr;
    }
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

void print_phdr(const Elf32_Phdr *phdr, int arg)
{
    printf("%-12s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %c%c%c %d\n",
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
