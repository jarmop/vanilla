function leftPad(str: string, n: number) {
    let pStr = str;
    while (pStr.length < n) {
        pStr = "0" + pStr;
    }
    return pStr;
}

/**
 * Only supports max 32 bits
 */
function leMax32(n: number, bits: number): string {
    const arr: number[] = [];
    for (let i = 0; i < bits; i += 8) {
        arr.push(n >> i);
    }

    return [...(new Uint8Array(arr))]
        .map((v) => leftPad(v.toString(16), 2))
        .join("");
}

function le(n: number, bits: number): string {
    if (bits > 32) {
        return leMax32(n, 32) + leftPad("", (bits - 32) / 4);
    }
    return leMax32(n, bits);
}

const headerFieldsList = {
    ehdr64: [
        [1, "EI_MAG0"],
        [1, "EI_MAG1"],
        [1, "EI_MAG2"],
        [1, "EI_MAG3"],
        [1, "EI_CLASS"],
        [1, "EI_DATA"],
        [1, "EI_VERSION"],
        [1, "EI_OSABI"],
        [1, "EI_ABIVERSION"],
        [7, "EI_PAD"],
        [2, "e_type"],
        [2, "e_machine"],
        [4, "e_version"],
        [8, "e_entry"],
        [8, "e_phoff"],
        [8, "e_shoff"],
        [4, "e_flags"],
        [2, "e_ehsize"],
        [2, "e_phentsize"],
        [2, "e_phnum"],
        [2, "e_shentsize"],
        [2, "e_shnum"],
        [2, "e_shstrndx"],
    ],
    phdr64: [
        [4, "p_type"],
        [4, "p_flags"],
        [8, "p_offset"],
        [8, "p_vaddr"],
        [8, "p_paddr"],
        [8, "p_filesz"],
        [8, "p_memsz"],
        [8, "p_align"],
    ],
    shdr64: [
        [4, "sh_name"],
        [4, "sh_type"],
        [8, "sh_flags"],
        [8, "sh_addr"],
        [8, "sh_offset"],
        [8, "sh_size"],
        [8, "sh_addralign"],
        [8, "sh_entsize"],
    ],
} as const;

const ELFCLASS64 = 2;
const ELFDATA2LSB = 1;
const EV_CURRENT = 1;
const SYSTEM_V = 0;
const ET_EXEC = 2;
const EM_X86_64 = 0x3E;

const baseVaddr = 0x400000;
const codeOff = 0x78;
const entryVaddr = baseVaddr + codeOff;
const phdr_off = 0x40;

const elfHeader: Record<string, Record<string, number>> = {
    ehdr64: {
        EI_MAG0: 0x7F,
        EI_MAG1: 0x45,
        EI_MAG2: 0x4C,
        EI_MAG3: 0x46,
        EI_CLASS: ELFCLASS64,
        EI_DATA: ELFDATA2LSB,
        EI_VERSION: EV_CURRENT,
        EI_OSABI: SYSTEM_V,
        EI_ABIVERSION: 0,
        EI_PAD: 0,
        e_type: ET_EXEC,
        e_machine: EM_X86_64,
        e_version: EV_CURRENT,
        e_entry: entryVaddr,
        e_phoff: phdr_off,
        e_shoff: 0,
        e_flags: 0,
        e_ehsize: 64,
        e_phentsize: 56,
        e_phnum: 1,
        e_shentsize: 0,
        e_shnum: 0,
        e_shstrndx: 0,
    },
};

const ehdrStr = headerFieldsList.ehdr64.map(([size, name]) => {
    const value = elfHeader.ehdr64[name];
    return le(value, size * 8);
});

const str = [
    [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x57],
    [0x6F, 0x72, 0x6C, 0x64, 0x21, 0x0D, 0x0A],
].flat();

const codeStr = [
    // mov eax,1  –  Put the linux syscall id for "write" (1) into the eax register
    [0xB8, 0x01, 0x00, 0x00, 0x00],
    // mov edi,1  –  Put the file id into the edi register (1 = stdout)
    [0xBF, 0x01, 0x00, 0x00, 0x00],
    // lea rsi,[rip+0x10]  –  Put the address of the string into the rsi register
    // (distance from the current instruction to the string is 16 bytes)
    // 0x8D is the op code for lea. 0x48 is a prefix telling that the operands are 64 bits.
    // 0x35 is the ModR/M byte for *SI destination register and 32-bit displacement
    // (Table 2-2 in chapter 2.1.3 of the x86 instruction reference manual)
    [0x48, 0x8D, 0x35, 0x10, 0x00, 0x00, 0x00],
    // mov edx,15  –  Put the size of the string into the edx register
    // "Hello, World!\n" is 15 bytes, "\n" is translated by linux into
    // two separate characters: line feed, and carriage return
    [0xBA, str.length, 0x00, 0x00, 0x00],
    [0x0F, 0x05], // syscall
    [0xB8, 0x3C, 0x00, 0x00, 0x00], // mov eax,60
    [0x31, 0xFF], // xor edi,edi
    [0x0F, 0x05], // syscall
    // "Hello, world!\n"
    ...str,
    // [0x48,0x65,0x6C,0x6C,0x6F,0x2C,0x20,0x57,0x6F,0x72,0x6C,0x64,0x21,0x0D,0x0A]
].flat().map((v) => le(v, 8));

const fileSize = codeOff + codeStr.length;

elfHeader.phdr64 = {
    p_type: 1,
    p_flags: 5,
    p_offset: 0,
    p_vaddr: baseVaddr,
    p_paddr: baseVaddr,
    p_filesz: fileSize,
    p_memsz: fileSize,
    p_align: 0x1000,
};

const phdrStr = headerFieldsList.phdr64.map(([size, name]) => {
    const value = elfHeader.phdr64[name];
    return le(value, size * 8);
});

const hexstr = [...ehdrStr, ...phdrStr, ...codeStr].join("");

await Deno.writeFile("foo.elf", new Uint8Array());

for (let i = 0; i < hexstr.length; i += 32) {
    const hexstr32 = hexstr.slice(i, i + 32);
    await Deno.writeFile("foo.elf", Uint8Array.fromHex(hexstr32), {
        append: true,
    });
}
