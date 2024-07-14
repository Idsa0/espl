#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>

#define MAX_CHOICE_LEN 64
#define INVALID_FILE (void *)-1

#define FREE(ptr)   \
    if (ptr)        \
    {               \
        free(ptr);  \
        ptr = NULL; \
    }

unsigned char debug = 0;
char *file_name = NULL;
FILE *file = INVALID_FILE;
char *mapped_file = INVALID_FILE;
int file_size = 0;
FILE *file_alt = INVALID_FILE;
int file_size_alt = 0;

typedef struct fun_desc
{
    char *name;
    void (*fun)();
} fun_desc;

void toggle_debug_mode()
{
    debug ^= 1;
    fprintf(stderr, "Debug flag now %s\n", debug ? "on" : "off");
}

void examine_elf_file()
{
    if (file == INVALID_FILE)
    {
        fprintf(stderr, "Invalid File!");
        return;
    }

    if (mapped_file != INVALID_FILE)
        munmap(mapped_file, file_size);
    mapped_file = mmap(file, file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);

    Elf32_Ehdr *header = (Elf32_Ehdr *)mapped_file;

    // 1. magic bytes
    printf("Magic bytes: %c%c%c\n", header->e_ident[1], header->e_ident[2], header->e_ident[3]);
    if (header->e_ident[1] != 'E' || header->e_ident[2] != 'L' || header->e_ident[3] != 'F')
    {
        fprintf(stderr, "Not an ELF file\n");
        return;
    }

    // 2. data encoding
    printf("Data encoding: %s\n", header->e_ident[EI_DATA] == 1 ? "little endian" : "big endian");

    // 3. entry point address
    printf("Entry point address: 0x%x\n", header->e_entry);

    // 4. file offset for section header
    printf("File offset for section header: %d\n", header->e_shoff);

    // 5. number of section headers
    printf("Number of section headers: %d\n", header->e_shnum);

    // 6. sections size
    for (int i = 0; i < header->e_shnum; ++i)
    {
        Elf32_Shdr *section = (Elf32_Shdr *)(mapped_file + header->e_shoff + i * header->e_shentsize);
        printf("Section %d: %x\n", i, section->sh_size);
    }
    // printf("Size of each section header: %d\n", header->e_shentsize);

    // 7. file offset for header table
    printf("File offset for header table: %d\n", header->e_phoff);

    // 8. number of header entries
    printf("Number of header entries: %d\n", header->e_phnum);

    // 9. size of each header entry
    for (int i = 0; i < header->e_phnum; ++i)
    {
        Elf32_Phdr *section = (Elf32_Phdr *)(mapped_file + header->e_phoff + i * header->e_phentsize);
        printf("Header %d: %x\n", i, section->p_filesz);
    }

    printf("Size of each header entry: %d\n", header->e_phentsize);

    return;
}

void print_section_names()
{
    if (mapped_file == INVALID_FILE)
    {
        fprintf(stderr, "Invalid File!");
        return;
    }

    Elf32_Ehdr *header = (Elf32_Ehdr *)mapped_file;
    Elf32_Shdr *section_headers = (Elf32_Shdr *)(mapped_file + header->e_shoff);
    Elf32_Half strtab_index = header->e_shstrndx;

    printf("File %s\n", file_name);
    for (int i = 0; i < header->e_shnum; ++i)
    {
        Elf32_Shdr *section = &section_headers[i];
        char *section_name = (char *)(mapped_file + section_headers[strtab_index].sh_offset + section->sh_name);
        printf("[%d]\t %-20s\t %8x\t %x\t %x\t %x\n", i, section_name, section->sh_addr, section->sh_offset, section->sh_size, section->sh_type);
    }
}

void print_symbols()
{
    // TODO stub method
}

void check_files_merge()
{
    // TODO stub method
}

void merge_elf_files()
{
    // TODO stub method
}

void quit()
{
    // FREE MEMORY IF NEEDED
    if (file != INVALID_FILE)
    {
        fclose(file);
    }
    // FREE(file_name); // no need to free yet

    if (mapped_file != INVALID_FILE)
    {
        munmap(mapped_file, file_size);
    }

    exit(0);
}

void load_file()
{
    if (file != INVALID_FILE)
    {
        munmap(file, file_size);
        fclose(file);
    }

    file = fopen(file_name, "r");
    if (file == INVALID_FILE)
    {
        perror("fopen");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    mapped_file = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
    if (file == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        exit(1);
    }

    file_name = argv[1];
    load_file();

    fun_desc functions[] =
        {{"Toggle Debug Mode", toggle_debug_mode},
         {"Examine ELF File", examine_elf_file},
         {"Print Section Names", print_section_names},
         {"Print Symbols", print_symbols},
         {"Check Files Merge", check_files_merge},
         {"Merge ELF Files", merge_elf_files},
         {"Quit", quit}};

    int bound = sizeof(functions) / sizeof(functions[0]);
    int choice;
    char input[MAX_CHOICE_LEN];
    char *endptr;
    while (!feof(stdin))
    {
        printf("Choose action:\n");
        for (int i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
            printf("%d-%s\n", i, functions[i].name);

        if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
        {
            choice = strtol(input, &endptr, 10);

            if (endptr == input || *endptr != '\n')
                // invalid input - for now we do nothing and continue
                fprintf(stderr, "Invalid input\n");

            else if (choice < 0 || choice >= bound)
                fprintf(stderr, "Not within bounds\n");

            else
                functions[choice].fun();
        }
    }

    return 0;
}
