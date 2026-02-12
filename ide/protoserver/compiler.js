const elfHeader = {
    e_ident: {
        EI_MAG0: 0x7F,
        EI_MAG1: 0x45,
        EI_MAG2: 0x4C,
        EI_MAG3: 0x46,
        EI_CLASS: 0,
EI_DATA: 0,
EI_VERSION: 0,
EI_OSABI: 0,
EI_ABIVERSION: 0,
EI_PAD: 0,
e_type: 0,
e_machine: 0,
e_version: 0,
e_entry: 0,
e_phoff: 0,
e_shoff: 0,
e_flags: 0,
e_ehsize: 0,
e_phentsize: 0,
e_phnum: 0,
e_shentsize: 0,
e_shnum: 0,
e_shstrndx: 0,

    },
};

// const foo = Uint8Array.fromHex("4845");

const foo = Uint8Array.fromHex(
    Object.values(elfHeader.e_ident).map((n) => n.toString(16)).join(""),
);

await Deno.writeFile("foo.elf", foo);

const baseVaddr = 0x400000;
const codeOff = 0x78;

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
}

headerFieldsList.ehdr64.forEach(([_, name]) => {
    console.log(`${name}: 0,`);
});