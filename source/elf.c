#include "elf.h"
#include "uart.h"
#include <stddef.h>
#include "platform.h"
#include "utility.h"
#include "memory.h"

static int elf_verify_header_ident(elf32_header* header)
{
    // Header size
    if (header->ehsize != sizeof(elf32_header))
    {
        uart_puts("Unexpected header size\n");
        return -1;
    }

    // Magic bytes
    int* identInt = (int*)(&header->ident[0]);
    if (*identInt != ELF_MAGIC)
    {
        uart_puts("Unexpected elf header\n");
        return -1;
    }

    // Version
    if (header->ident[6] != EV_CURRENT)
    {
        uart_puts("Invalid elf version\n");
        return -1;
    }

    // Max segments
    if (header->phnum > EXE_MAX_SEGMENTS)
    {
        uart_puts("Too many segments in header\n");
        return -1;
    }

    // Machine type
    if (header->machine != EM_ARM)
    {
        uart_puts("Invalid machine type\n");
        return -1;
    }

    return 0;
}

int elf_get_func_info(char* elf, int elf_size, func_info** info)
{
    elf32_header* header = (elf32_header*)elf;

    if (elf_verify_header_ident(header) != 0)
        return -1;

    elf_shdr* shdrs = (elf_shdr*)&elf[header->shoff];

    // Find the string table so we can resolve function names
    char* strtab = NULL;
    int numSymbols = 0;
    elf_sym* symtab = NULL;
    unsigned int i;
    for (i = 0; i < header->shnum; i++)
    {
        // TODO: This doesn't deal well with multiple STRTAB/SYMTAB
        if (shdrs[i].type == SHT_STRTAB && i != header->shstrndx)
        {
            strtab = elf + shdrs[i].offset;
        }

        if (shdrs[i].type == SHT_SYMTAB)
        {
            symtab = (elf_sym*)(elf + shdrs[i].offset);
            numSymbols = shdrs[i].size / shdrs[i].entsize;
        }
    }

    // Count number of functions
    int numFunctions = 0;
    for (i = 0; i < numSymbols; i++)
    {
        if (ELF32_ST_TYPE(symtab[i].info) == STT_FUNC && symtab[i].name > 0)
            numFunctions++;
    }

    // We now know the number of functions, we have the symbol table and the string table
    // Time to combine it all into the result
    *info = (func_info*)palloc(numFunctions * sizeof(func_info));
    int curFuncIndex = 0;
    for (i = 0; i < numSymbols; i++)
    {
        // Skip irrelevant symbols
        if (ELF32_ST_TYPE(symtab[i].info) != STT_FUNC || symtab[i].name == 0)
            continue;

        elf_sym* sym = &symtab[i];

        func_info* curFun = &(*info)[curFuncIndex++];
        curFun->address = sym->value;

        char* name = strtab + sym->name;
        int nameLen = my_strlen(name);

        // Add in an extra for null termination
        curFun->name = (char*)palloc(nameLen + 1);
        
        memcpy(curFun->name, name, nameLen);

        // Make sure it's terminated nicely
        curFun->name[nameLen] = 0;
    }

    return numFunctions; //curFuncIndex + 1;
}

int elf_load(char* file, int file_size)
{
    elf32_header* header = (elf32_header*)file;

    int highestPAddr = 0;
    int elfEndAddr = 0;

    unsigned int i;
    for (i = 0; i < header->phnum; i++)
    {
        elf_ph* ph = (elf_ph*)(file + header->phoff + (i * header->phentsize));

        if (ph->type != PT_LOAD)
            continue;

        if (ph->paddr > highestPAddr)
        {
            highestPAddr = ph->paddr;
            elfEndAddr = ph->paddr + ph->memsz;
        }

        // Load section into memory
        memcpy((char*)ph->paddr, &file[ph->offset], ph->filesz);

        // TODO: Need to make this way more clever, fill memory with 0's if file size
        // Isn't equal to memory size etc
    }

    return elfEndAddr;
}