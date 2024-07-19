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

unsigned char loaded = 0;
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
    if (files[loaded] != INVALID_FILE)
    {
        return;
    }

    files[loaded] = fopen(filename, "r");
    if (files[loaded] == INVALID_FILE)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(files[loaded], 0, SEEK_END);
    file_sizes[loaded] = ftell(files[loaded]);
    fseek(files[loaded], 0, SEEK_SET);

    mapped_files[loaded] = mmap(NULL, file_sizes[loaded], PROT_READ, MAP_PRIVATE, fileno(files[loaded]), 0);
    if (files[loaded] == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (loaded != MAX_FILES)
        ++loaded;
}

static inline void get_and_load_next(void)
{
    if (loaded >= MAX_FILES)
    {
        fprintf(stderr, "No more files can be loaded\n");
        return;
    }

    filenames[loaded] = get_filename();
    load_file(filenames[loaded]);
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
    if (!loaded)
        get_and_load_next();

    for (int i = 0; i < loaded; ++i)
    {
        printf("File %s\n", filenames[i]);
        pprint_phdr((Elf32_Ehdr *)mapped_files[i]);
    }
}

void print_section_names(void)
{
    if (!loaded)
        get_and_load_next();

    for (int i = 0; i < loaded; ++i)
    {
        Elf32_Ehdr *header = (Elf32_Ehdr *)mapped_files[i];
        Elf32_Shdr *shdr = (Elf32_Shdr *)(mapped_files[i] + header->e_shoff);
        char *strTab = mapped_files[i] + shdr[header->e_shstrndx].sh_offset;

        printf("File: %s\n", filenames[i]);

        for (int j = 0; j < header->e_shnum; ++j)
        {
            printf("[%d] %s %08x %06x %06x %d\n", j,
                   strTab + shdr[j].sh_name, shdr[j].sh_addr,
                   shdr[j].sh_offset, shdr[j].sh_size, shdr[j].sh_type);
        }
        printf("\n");
    }
}

void print_symbols(void)
{
    if (!loaded)
        get_and_load_next();

    Elf32_Ehdr *header;
    Elf32_Shdr *shdrTable, *sectionHeader;
    Elf32_Sym *symTable, *entry;
    Elf32_Addr value;
    char sh_index[100], *strTab, *sh_name, *symbol_names, *s_name;

    // iterate over all the files
    for (int i = 0; i < loaded; ++i)
    {
        if (!mapped_files[i])
            continue;

        header = (Elf32_Ehdr *)mapped_files[i];
        shdrTable = (Elf32_Shdr *)(mapped_files[i] + header->e_shoff);
        strTab = mapped_files[i] + shdrTable[header->e_shstrndx].sh_offset;

        printf("File %s\n", filenames[i]);

        // iterate over all the sections
        for (int j = 0; j < header->e_shnum; ++j)
        {
            if (shdrTable[j].sh_type == SHT_SYMTAB || shdrTable[j].sh_type == SHT_DYNSYM)
            {
                symTable = (Elf32_Sym *)(mapped_files[i] + shdrTable[j].sh_offset);
                symbol_names = (char *)(mapped_files[i] + shdrTable[shdrTable[j].sh_link].sh_offset);
                printf("[index] value section_index section_name symbol_name\n");

                // iterate over all the symbols
                for (int k = 0; k < shdrTable[j].sh_size / sizeof(Elf32_Sym); ++k)
                {
                    entry = &symTable[k];

                    value = entry->st_value;
                    if (entry->st_shndx == SHN_UNDEF)
                        strcpy(sh_index, "UND");
                    else if (entry->st_shndx == SHN_ABS)
                        strcpy(sh_index, "ABS");
                    else
                        sprintf(sh_index, "%d", entry->st_shndx);

                    sh_name = entry->st_shndx == SHN_UNDEF || entry->st_shndx == SHN_ABS ? "" : strTab + shdrTable[entry->st_shndx].sh_name;
                    s_name = entry->st_name == 0 ? "" : symbol_names + entry->st_name;

                    if (entry->st_info == STT_SECTION)
                    {
                        sectionHeader = &shdrTable[entry->st_shndx];
                        s_name = (char *)(mapped_files[0] + shdrTable[header->e_shstrndx].sh_offset + sectionHeader->sh_name);
                    }
                    printf("[%02d]  %08x %-3s %-20s %-20s\n", k, value, sh_index, sh_name, s_name);
                }

                printf("\n");
            }
        }
    }
}

static inline unsigned char search_symbol(const char *name,
                                Elf32_Sym *symtab,
                                const Elf32_Word size,
                                char *symbol_names,
                                Elf32_Shdr *shdr,
                                const Elf32_Ehdr *header)
{
    Elf32_Shdr *sectionHeader;
    Elf32_Sym *entry;
    char *symbolName;

    for (int i = 0; i < size; ++i)
    {
        entry = &symtab[i];
        symbolName = symbol_names + entry->st_name;
        if (entry->st_info == STT_SECTION)
        {
            sectionHeader = &shdr[entry->st_shndx];
            symbolName = (char *)(mapped_files[0] + shdr[header->e_shstrndx].sh_offset + sectionHeader->sh_name);
        }

        if (strcmp(name, symbolName) == 0 && symtab[i].st_shndx != SHN_UNDEF)
            return 1;
    }

    return 0;
}

void check_files_merge(void)
{
    // explicitly support only 2 files - no time to implement properly
    while (loaded < 2)
        get_and_load_next();

    Elf32_Ehdr *header, *header2;
    Elf32_Shdr *shdrTable1, *shdrTable2, *shdr;
    Elf32_Sym *symtab1, *symtab2, *entry;
    int sectionIndex, count1 = 0, count2 = 0, symtab1_index = -1, symtab2_index = -1;
    char *symbols_names1, *symbols_names2, *symbolName;

    header = (Elf32_Ehdr *)mapped_files[0];
    header2 = (Elf32_Ehdr *)mapped_files[1];

    shdrTable1 = (Elf32_Shdr *)(mapped_files[0] + header->e_shoff);
    shdrTable2 = (Elf32_Shdr *)(mapped_files[1] + header2->e_shoff);

    for (int i = 0; i < header->e_shnum; ++i)
        if (shdrTable1[i].sh_type == SHT_SYMTAB || shdrTable1[i].sh_type == SHT_DYNSYM)
        {
            ++count1;
            symtab1_index = i;
        }

    for (int i = 0; i < header2->e_shnum; ++i)
        if (shdrTable2[i].sh_type == SHT_SYMTAB || shdrTable2[i].sh_type == SHT_DYNSYM)
        {
            ++count2;
            symtab2_index = i;
        }

    if (count1 != 1 || count2 != 1)
    {
        fprintf(stderr, "Invalid number of symbol tables\n");
        return;
    }

    symtab1 = (Elf32_Sym *)(mapped_files[0] + shdrTable1[symtab1_index].sh_offset);
    symtab2 = (Elf32_Sym *)(mapped_files[1] + shdrTable2[symtab2_index].sh_offset);
    symbols_names1 = (char *)(mapped_files[0] + shdrTable1[shdrTable1[symtab1_index].sh_link].sh_offset);
    symbols_names2 = (char *)(mapped_files[1] + shdrTable2[shdrTable2[symtab2_index].sh_link].sh_offset);

    for (int i = 1; i < shdrTable1[symtab1_index].sh_size / sizeof(Elf32_Sym); ++i)
    {
        entry = &symtab1[i];
        if (entry->st_info == STT_SECTION)
        {
            sectionIndex = entry->st_shndx;
            shdr = &shdrTable1[sectionIndex];
            symbolName = (char *)(mapped_files[0] + shdrTable1[header->e_shstrndx].sh_offset + shdr->sh_name);
        }
        else
            symbolName = symbols_names1 + entry->st_name;

        if (symtab1[i].st_shndx == SHN_UNDEF &&
            search_symbol(symbolName, symtab2, shdrTable2[symtab2_index].sh_size / sizeof(Elf32_Sym), symbols_names2, shdrTable2, header2) == 0)
            fprintf(stderr, "Symbol %s not defined\n", symbolName);

        else if (symtab1[i].st_shndx != SHN_UNDEF &&
                 search_symbol(symbolName, symtab2, shdrTable2[symtab2_index].sh_size / sizeof(Elf32_Sym), symbols_names2, shdrTable2, header2) == 1)
            fprintf(stderr, "Symbol %s defined in both files\n", symbolName);
    }
}

void merge_elf_files(void)
{
    fprintf(stderr, "Not implemented!\n");
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
