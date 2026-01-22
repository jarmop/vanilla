import { bytes } from "./bin";
import { leftPad } from "./helper";

const elfStructures = {
    ehdr64: {
        0: [1, "EI_MAG0"],
        1: [1, "EI_MAG1"],
        2: [1, "EI_MAG2"],
        3: [1, "EI_MAG3"],
        4: [1, "EI_CLASS"],
        5: [1, "EI_DATA"],
        6: [1, "EI_VERSION"],
        7: [1, "EI_OSABI"],
        8: [1, "EI_ABIVERSION"],
        9: [7, "EI_PAD"],
        16: [2, "e_type"],
        18: [2, "e_machine"],
        20: [4, "e_version"],
        24: [8, "e_entry"],
        32: [8, "e_phoff"],
        40: [8, "e_shoff"],
        48: [4, "e_flags"],
        52: [2, "e_ehsize"],
        54: [2, "e_phentsize"],
        56: [2, "e_phnum"],
        58: [2, "e_shentsize"],
        60: [2, "e_shnum"],
        62: [2, "e_shstrndx"],
    },
    phdr64: {
        0: [4, "p_type"],
        4: [4, "p_flags"],
        8: [8, "p_offset"],
        16: [8, "p_vaddr"],
        24: [8, "p_paddr"],
        32: [8, "p_filesz"],
        40: [8, "p_memsz"],
        48: [8, "p_align"],
    },
} as const;

type ElfStructures = typeof elfStructures;

export type ElfData = {
    bytes: number[];
    name: string;
    offset: number;
    // value: string | number,
};

function isValidOffset<T extends Record<number, any>>(
    offset: number | keyof T,
    elfStructure: T,
): offset is keyof T {
    return offset in elfStructure;
}

function parseElf(
    elfStructure: ElfStructures[keyof ElfStructures],
    initialOffset = 0,
) {
    const offsets = Object.keys(elfStructure).map((k) => parseInt(k));
    const maxOffset = initialOffset + offsets[offsets.length - 1];
    const elfDataList: ElfData[] = [];
    let unknownOffset = 0;
    let offset = initialOffset;
    while (offset <= maxOffset) {
        const relativeOffset = offset - initialOffset;
        const field = isValidOffset(relativeOffset, elfStructure)
            ? elfStructure[relativeOffset]
            : false;
        if (field) {
            if (unknownOffset) {
                // First record the possible sequence of unidentified bytes
                const size = offset - unknownOffset;
                elfDataList.push({
                    name: "?",
                    offset: unknownOffset,
                    bytes: bytes.slice(unknownOffset, unknownOffset + size),
                });
                unknownOffset = 0;
            }
            // Then add the identified bytes
            const [size, name] = field;
            const dataBytes = bytes.slice(offset, offset + size);
            const elfDatum = {
                name,
                offset,
                bytes: dataBytes,
            };
            elfDataList.push(elfDatum);
            offset += size;
            continue;
        } else {
            // Only store the first unknown offset in a sequence of unknown offsets
            unknownOffset = unknownOffset || offset;
            offset++;
        }
    }

    const isLittleEndian = bytes[5] == 1;
    if (isLittleEndian) {
        elfDataList.forEach(({ bytes }) => bytes.reverse());
    }

    return elfDataList;
}

export function bytesToNum(bytes: number[]) {
    const hex = bytes.map((b) => leftPad(b.toString(16), 2)).join("");
    return parseInt(hex, 16);
}

function getValue(data: ElfData[], field: string) {
    const bytes = data.find((d) => d.name == field)?.bytes || [];
    // const hex = bytes.map((b) => leftPad(b.toString(16), 2)).join("");
    // const num = parseInt(hex, 16);
    // return num;
    return bytesToNum(bytes);
}

const elfHeaderSize = 64;
const ehdr = parseElf(elfStructures.ehdr64);
const startOfProgramHeaders = getValue(ehdr, "e_phoff");
const programHeaderSize = getValue(ehdr, "e_phentsize");
const numberOfProgramHeaders = getValue(ehdr, "e_phnum");
const startOfSectionHeaders = getValue(ehdr, "e_shoff");
const numberOfSectionHeaders = getValue(ehdr, "e_shnum");

const phdr = parseElf(elfStructures.phdr64, elfHeaderSize);

// const phdr2 = parseElf(elfStructures.phdr64, elfHeaderSize + programHeaderSize);

// console.log(phdr2);

const programHeaders = [];
let ptOffset = elfHeaderSize;
for (let i = 0; i < numberOfProgramHeaders; i++) {
    programHeaders.push(parseElf(elfStructures.phdr64, ptOffset));
    ptOffset += programHeaderSize;
}

const foo = programHeaders.filter((phdr) => {
    return getValue(phdr, "p_type") == 1;
});
console.log(foo);

const e_entry = getValue(ehdr, "e_entry");
const p_offset = getValue(phdr, "p_offset");
const p_vaddr = getValue(phdr, "p_vaddr");
// Have to check that the PT_LOAD = 1 (Loadable segment)
const codeOffset = p_offset + (e_entry - p_vaddr);
const codeSize = getValue(phdr, "p_filesz");

export const elfMeta = {
    codeOffset,
    codeSize,
    ehdr,
    phdr,
    programHeaders,
    startOfProgramHeaders,
    numberOfProgramHeaders,
    startOfSectionHeaders,
    numberOfSectionHeaders,
};

// const instructions = bytes
