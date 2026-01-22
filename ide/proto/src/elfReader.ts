import { binstr, bytes } from "./bin";
import { leftPad } from "./helper";

// type ElfStructure = Record<
//     "ehdr64" | "phdr64",
//     Record<number, [number, string]>
// >;
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

type ElfData = {
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

export const ehdr = parseElf(elfStructures.ehdr64);
// export const ehdr = [];
// const p_offset = ehdr.find(d => )
export const phdr = parseElf(elfStructures.phdr64, 64);

const ELF_MAGIC = [127, 69, 76, 70];

function isElf(bytes: number[]) {
    return bytes.slice(0, 4).every((byte, i) => byte === ELF_MAGIC[i]);
}

const codeStart = 0;
/**
 * Assume 64-bit, little-endian
 * Todo: Handle endianness and "bitness"
 */
binstr.slice(24, 8).split("").forEach((hex) => {
    // const num = parseInt(hex, 16);
    // for (let i = 0; i < 8; i++) {
    //     codeStart += b * Math.pow();
    // }
});

// console.log(binstr.slice(24, 8));
// console.log(binstr.slice(24 * 2, 24 * 2 + 8 * 2));
// const str = binstr.slice(24 * 2, 24 * 2 + 8 * 2);
const entryHex = bytes
    .slice(24, 24 + 8)
    .reverse()
    .map((n) => leftPad(n.toString(16), 2))
    .join("");
// console.log(entryHex);
// console.log(parseInt(entryHex, 16).toString(16));

export const elfData2 = {
    type: isElf(bytes) ? "ELF" : "?",
    bits: bytes[4] == 1 ? 32 : 64,
    endianness: bytes[5] == 1 ? "little-endian" : "big-endian",
    elfVersion: bytes[6],
    osabi: bytes[7] == 3 ? "Linux" : "?",
    abiVersion: bytes[8],
    phdrStart: bytes[32].toString(16),
    codeStart: parseInt(entryHex, 16).toString(16),
};
