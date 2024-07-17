#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>
#include <string.h>

#define MAX_CHOICE_LEN 64
#define INVALID_FILE (void *)-1
#define MAX_FILES 2

#define FREE(ptr)   \
    if (ptr)        \
    {               \
        free(ptr);  \
        ptr = NULL; \
    }

unsigned char next = 0;
unsigned char debug = 0;
char *filenames[MAX_FILES] = {NULL};
FILE *files[MAX_FILES] = {INVALID_FILE};
char *mapped_files[MAX_FILES] = {INVALID_FILE};
int file_sizes[MAX_FILES] = {0};

static inline char *get_filename(void)
{
    char *filename = (char *)malloc(MAX_CHOICE_LEN);
    if (!filename)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    printf("Enter file name: ");
    if (fgets(filename, MAX_CHOICE_LEN, stdin) == NULL)
    {
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    filename[strcspn(filename, "\n")] = '\0';

    return filename;
}

static inline void load_file(const char *filename)
{
    if (files[next] != INVALID_FILE)
    {
        return;
    }

    files[next] = fopen(filename, "r");
    if (files[next] == INVALID_FILE)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(files[next], 0, SEEK_END);
    file_sizes[next] = ftell(files[next]);
    fseek(files[next], 0, SEEK_SET);

    mapped_files[next] = mmap(NULL, file_sizes[next], PROT_READ, MAP_PRIVATE, fileno(files[next]), 0);
    if (files[next] == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (next != MAX_FILES - 1)
        ++next;
}

static inline void get_and_load_next(void)
{
    if (next >= MAX_FILES)
    {
        fprintf(stderr, "No more files can be loaded\n");
        return;
    }

    filenames[next] = get_filename();
    load_file(filenames[next]);
}

typedef struct fun_desc
{
    char *name;
    void (*fun)(void);
} fun_desc;

void toggle_debug_mode(void)
{
    debug ^= 1;
    fprintf(stderr, "Debug flag now %s\n", debug ? "on" : "off");
}

static inline void pprint_phdr(const Elf32_Ehdr *header)
{
    if (header == INVALID_FILE)
        return;

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
        Elf32_Shdr *section = (Elf32_Shdr *)((unsigned char *)header + header->e_shoff + i * header->e_shentsize);
        printf("Section %d: %x\n", i, section->sh_size);
    }

    // 7. file offset for header table
    printf("File offset for header table: %d\n", header->e_phoff);

    // 8. number of header entries
    printf("Number of header entries: %d\n", header->e_phnum);

    // 9. size of each header entry
    for (int i = 0; i < header->e_phnum; ++i)
    {
        Elf32_Phdr *program = (Elf32_Phdr *)((unsigned char *)header + header->e_phoff + i * header->e_phentsize);
        printf("Header %d: %x\n", i, program->p_filesz);
    }

    printf("Size of each header entry: %d\n", header->e_phentsize);
}

void examine_elf_file(void)
{
    if (!next)
        get_and_load_next();

    for (int i = 0; i < next; ++i)
    {
        printf("File %s\n", filenames[i]);
        pprint_phdr((Elf32_Ehdr *)mapped_files[i]);
    }
}

static inline void pprint_section_names(const Elf32_Ehdr *header)
{
    if (header == INVALID_FILE)
        return;

    Elf32_Shdr *section_headers = (Elf32_Shdr *)(header + header->e_shoff);
    Elf32_Half strtab_index = header->e_shstrndx;

    for (int i = 0; i < header->e_shnum; ++i)
    {
        Elf32_Shdr *section = &section_headers[i];
        char *section_name = (char *)(header + section_headers[strtab_index].sh_offset + section->sh_name);
        printf("[%d]\t %-20s\t %8x\t %x\t %x\t %x\n", i, section_name, section->sh_addr, section->sh_offset, section->sh_size, section->sh_type);
    }
}

void print_section_names(void)
{
    if (!next)
        get_and_load_next();

    for (int i = 0; i < next; ++i)
    {
        printf("File %s\n", filenames[i]);
        pprint_section_names((Elf32_Ehdr *)mapped_files[i]);
    }
}

static inline void pprint_symbols(const Elf32_Ehdr *header)
{
    if (header == INVALID_FILE || !header || !header->e_shnum)
        return;

    Elf32_Shdr *section_headers = (Elf32_Shdr *)(header + header->e_shoff);
    Elf32_Shdr *symtab = NULL;
    Elf32_Shdr *strtab = NULL;

    for (int i = 0; i < header->e_shnum; ++i)
    {
        Elf32_Shdr *section = &section_headers[i];
        if (section->sh_type == SHT_SYMTAB)
            symtab = section;
        else if (section->sh_type == SHT_STRTAB)
            strtab = section;
    }

    if (!symtab || !strtab)
    {
        fprintf(stderr, "No symbol table or string table found\n");
        return;
    }

    Elf32_Sym *symbols = (Elf32_Sym *)(header + symtab->sh_offset);
    char *strtab_data = (char *)(header + strtab->sh_offset);

    for (int i = 0; i < symtab->sh_size / symtab->sh_entsize; ++i)
    {
        Elf32_Sym *symbol = &symbols[i];
        printf("[%d]\t %x\t %x\t %s\n", i, symbol->st_value, symbol->st_shndx, strtab_data + symbol->st_name);
    }

    printf("\n");

    // TODO this does not work yet!
}

/**
 * The new Print Symbols option, for each open ELF file, should visit all the symbols in that ELF file (if none, print an error message and return). For each symbol, print its index number, its name and the name of the section in which it is defined. (similar to readelf -s). Format should be:

File ELF-file0name

[index] value section_index section_name symbol_name
[index] value section_index section_name symbol_name
[index] value section_index section_name symbol_name
 */
void print_symbols(void)
{
    if (!next)
        get_and_load_next();

    for (int i = 0; i < next; ++i)
    {
        printf("File %s\n", filenames[i]);
        pprint_symbols((Elf32_Ehdr *)mapped_files[i]);
    }
}

void check_files_merge(void)
{
    // TODO stub method
}

void merge_elf_files(void)
{
    // TODO stub method
}

void quit(void)
{
    for (int i = 0; i < MAX_FILES; ++i)
    {
        if (files[i] != INVALID_FILE)
            fclose(files[i]);

        if (mapped_files[i] != INVALID_FILE)
            munmap(mapped_files[i], file_sizes[i]);

        FREE(filenames[i]);
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    for (int i = 0; i < MAX_FILES; ++i)
    {
        filenames[i] = NULL;
        files[i] = INVALID_FILE;
        mapped_files[i] = INVALID_FILE;
        file_sizes[i] = 0;
    }

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
