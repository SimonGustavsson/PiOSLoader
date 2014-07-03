#ifndef ELF_H
#define ELF_H

#define EI_NIDENT 16
#define ELF_MAGIC 0x464C457F
#define EXE_MAX_SEGMENTS 200
#define EM_ARM 40
#define ELF32_ST_TYPE(i) ((i)& 0xF)

typedef struct {
    unsigned char  ident[EI_NIDENT];
    unsigned short type;              // See elf_type
    unsigned short machine;
    unsigned short version;
    int            entry;             // Virtual address in memory of entry point to start execution
    int            phoff;             // Program header table file offset (in bytes)
    int            shoff;             // Section header table file offset (in bytes)
    int            flags;             // See elf_flags (note: flags are architecture specific)
    unsigned short ehsize;            // Header size (in bytes)
    unsigned short phentsize;         // Program header table entry size (in bytes)
    unsigned short phnum;             // Program header table entry count
    unsigned short shentsize;         // Section header table entry size (in bytes)
    unsigned short shnum;             // Section header table entry count
    unsigned short shstrndx;          // Section header table index of entry associated with the section name string table
} elf32_header;

typedef struct {
    unsigned int type;
    unsigned int offset;
    unsigned int vaddr;
    unsigned int paddr;
    unsigned int filesz;
    unsigned int memsz;
    unsigned int flags;
    unsigned int align;
} elf_ph;

typedef enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7FFFFFFF
} elf_segtype;

typedef enum {
    EV_NONE = 0, // Invalid Version
    EV_CURRENT = 1, // Current Version
} elf_version;

typedef struct {
    unsigned int name;    
    unsigned int type;    
    unsigned int flags;   
    unsigned int addr;    
    unsigned int offset;  
    unsigned int size;    
    unsigned int link;    
    unsigned int info;    
    unsigned int addralign;
    unsigned int entsize;
} elf_shdr;

typedef struct {
    unsigned int   name;
    unsigned int   value;
    unsigned int   size;
    unsigned char  info;
    unsigned char  other;
    unsigned short shndx;
} elf_sym;

typedef enum {
    SHT_NULL = 0,
    SHT_PROGBITS = 1,
    SHT_SYMTAB = 2,
    SHT_STRTAB = 3,
    SHT_RELA = 4,
    SHT_HASH = 5,
    SHT_DYNAMIC = 6,
    SHT_NOTE = 7,
    SHT_NOBITS = 8,
    SHT_REL = 9,
    SHT_SHLIB = 10,
    SHT_DYNSYM = 11,
    SHT_LOPROC = 0x70000000,
    SHT_HIPROC = 0x7FFFFFFF,
    SHT_LOUSER = 0x80000000,
    SHT_HIUSER = 0xFFFFFFFF
} elf_shtype;

typedef enum {
    STT_NOTYPE = 0,
    STT_OBJECT = 1,
    STT_FUNC = 2,
    STT_SECTION = 3,
    STT_FILE = 4,
    STT_LOPROC = 13,
    STT_HIPROC = 15
} elf_stt;

typedef struct {
    char* name;
    unsigned int address;
} func_info;


// Reads all function names and their address from an in-memory ELF file
// and returns the number of functions found
int elf_get_func_info(char* elf, int elf_size, func_info** info);

/*
Loads the given ELF file into memory. (NOTE: This loads the ELF to whereever it wants to be,
    For it to work on the Rpi however, it MUST be 0x8000!)
    file: Pointer to an ELF file in memory
    file_size: The size of the file (in bytes).
    Returns: 0 on success, otherwise an error code.
*/
int elf_load(char* file, int file_size);

#endif