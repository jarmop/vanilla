// emit_elf.c: hard-coded "compiler" that outputs an ELF64 x86-64 executable.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void w8 (FILE *f, uint8_t  v) {
    if (fputc(v, f) == EOF) {
        perror("fputc"); exit(1);
    }
}

static void w16(FILE *f, uint16_t v) {
    for (int i=0;i<2;i++) {
        w8(f, (uint8_t)(v >> (8*i)));
    }
}

static void w32(FILE *f, uint32_t v) {
    for (int i=0;i<4;i++) {
        w8(f, (uint8_t)(v >> (8*i))); 
    }
}

static void w64(FILE *f, uint64_t v) {
    for (int i=0;i<8;i++) {
        w8(f, (uint8_t)(v >> (8*i))); 
    }
}

int main(void) {
    const char *out = "compiled";
    FILE *f = fopen(out, "wb");
    if (!f) { perror("fopen"); return 1; }

    // Layout:
    // 0x00..0x3f : ELF header (64 bytes)
    // 0x40..0x77 : Program header (56 bytes)
    // 0x78..     : Code+data (we place code at file offset 0x78)
    //
    // We load the whole file as one RX segment at vaddr 0x400000.
    const uint64_t base_vaddr = 0x400000;
    // const uint64_t ehdr_off   = 0x0;
    const uint64_t phdr_off   = 0x40;
    const uint64_t code_off   = 0x78;
    const uint64_t entry_vaddr = base_vaddr + code_off;

    // Machine code (x86-64 Linux syscalls):
    //   mov eax, 1          ; sys_write
    //   mov edi, 1          ; fd=1 (stdout)
    //   lea rsi, [rip+msg]  ; buf
    //   mov edx, 1          ; len=1
    //   syscall
    //   mov eax, 60         ; sys_exit
    //   xor edi, edi        ; status=0
    //   syscall
    //
    // msg: db "Hello, World!\n"
    //
    // Encoding (position-independent using RIP-relative LEA):

    /**
     * B8 is the base op code for move instructions that move an immediate value to a register
     * Apparently the register type is determined by an "offset" that is added to the opcode
     * B8 + 0 = *AX
     * B8 + 7 = BF = *DI
     */
    static const uint8_t code[] = {
        // mov eax,1  –  Put the linux syscall id for "write" (1) into the eax register
        0xB8,0x01,0x00,0x00,0x00,
        // mov edi,1  –  Put the file id into the edi register (1 = stdout)
        0xBF,0x01,0x00,0x00,0x00,
        // lea rsi,[rip+0x10]  –  Put the address of the string into the rsi register
        // (distance from the current instruction to the string is 16 bytes)
        // 0x8D is the op code for lea. 0x48 is a prefix telling that the operands are 64 bits.
        // 0x35 is the ModR/M byte for *SI destination register and 32-bit displacement 
        // (Table 2-2 in chapter 2.1.3 of the x86 instruction reference manual)
        0x48,0x8D,0x35,0x10,0x00,0x00,0x00,   
        // mov edx,15  –  Put the size of the string into the edx register
        // "Hello, World!\n" is 15 bytes, "\n" is translated by linux into  
        // two separate characters: line feed, and carriage return
        0xBA,0x0F,0x00,0x00,0x00,
        0x0F,0x05,                            // syscall
        0xB8,0x3C,0x00,0x00,0x00,             // mov eax,60
        0x31,0xFF,                            // xor edi,edi
        0x0F,0x05,                            // syscall
        // "Hello, world!\n"
        0x48,0x65,0x6C,0x6C,0x6F,0x2C,0x20,0x57,0x6F,0x72,0x6C,0x64,0x21,0x0D,0x0A
    };
    const uint64_t file_size = code_off + (uint64_t)sizeof(code);

    // ---------------- ELF header (Elf64_Ehdr) ----------------
    // e_ident
    w8(f, 0x7F); w8(f, 'E'); w8(f, 'L'); w8(f, 'F'); // magic
    w8(f, 2);     // EI_CLASS = ELFCLASS64
    w8(f, 1);     // EI_DATA  = ELFDATA2LSB
    w8(f, 1);     // EI_VERSION = EV_CURRENT
    w8(f, 0);     // EI_OSABI = System V
    w8(f, 0);     // EI_ABIVERSION
    for (int i=0;i<7;i++) w8(f, 0); // padding to 16 bytes

    w16(f, 2);          // e_type = ET_EXEC
    w16(f, 0x3E);       // e_machine = EM_X86_64
    w32(f, 1);          // e_version = EV_CURRENT
    w64(f, entry_vaddr);// e_entry
    w64(f, phdr_off);   // e_phoff
    w64(f, 0);          // e_shoff (no section headers)
    w32(f, 0);          // e_flags
    w16(f, 64);         // e_ehsize
    w16(f, 56);         // e_phentsize
    w16(f, 1);          // e_phnum
    w16(f, 0);          // e_shentsize
    w16(f, 0);          // e_shnum
    w16(f, 0);          // e_shstrndx

    // ---------------- Program header (Elf64_Phdr) ----------------
    // Single PT_LOAD segment: map [0..file_size) at vaddr 0x400000 as RX.
    w32(f, 1);          // p_type = PT_LOAD
    w32(f, 5);          // p_flags = PF_R | PF_X
    w64(f, 0);          // p_offset
    w64(f, base_vaddr); // p_vaddr
    w64(f, base_vaddr); // p_paddr (ignored)
    w64(f, file_size);  // p_filesz
    w64(f, file_size);  // p_memsz
    w64(f, 0x1000);     // p_align (page size)

    // ---------------- Code/data ----------------
    // Ensure we're at code_off (0x78). We should be at 0x40 + 56 = 0x78 already.
    long pos = ftell(f);
    if (pos < 0) { perror("ftell"); return 1; }
    if ((uint64_t)pos != code_off) {
        fprintf(stderr, "internal layout error: pos=%ld expected=%llu\n",
                pos, (unsigned long long)code_off);
        return 1;
    }

    if (fwrite(code, 1, sizeof(code), f) != sizeof(code)) {
        perror("fwrite");
        return 1;
    }

    if (fclose(f) != 0) { perror("fclose"); return 1; }

    // Make it executable: rely on user to chmod, to keep this "minimal C".
    printf("Wrote %s (%llu bytes). Now run:\n", out, (unsigned long long)file_size);
    printf("  chmod +x %s\n  ./%s\n", out, out);
    return 0;
}
