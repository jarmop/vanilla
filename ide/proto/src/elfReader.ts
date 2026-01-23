import { bytes } from "./bin";
import { leftPad } from "./helper";

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

type HeaderFieldsList = typeof headerFieldsList;
type HeaderFields = HeaderFieldsList[keyof HeaderFieldsList];
type HdrFieldNames = HeaderFields[number][1];
type EhdrFieldNames = HeaderFieldsList["ehdr64"][number][1];
type PhdrFieldNames = HeaderFieldsList["phdr64"][number][1];
type ShdrFieldNames = HeaderFieldsList["shdr64"][number][1];

const isLittleEndian = bytes[5] == 1;

export function bytesToNum(bytes: number[]): number {
    const hex = bytes.map((b) => leftPad(b.toString(16), 2)).join("");
    return parseInt(hex, 16);
}

function parseElf2<T extends HdrFieldNames>(
    hdrFields: HeaderFields,
    offset = 0,
) {
    const hdr: ElfData[] = [];
    const hdrMap: Record<T, number> = {} as Record<T, number>;
    hdrFields.forEach(([size, name]) => {
        const value = bytes.slice(offset, offset + size);
        if (isLittleEndian) {
            value.reverse();
        }
        hdr.push({
            name,
            offset,
            bytes: value,
        });
        hdrMap[name as T] = bytesToNum(value);
        offset += size;
    });

    return { hdr, hdrMap };
}

export type ElfData = {
    bytes: number[];
    name: string;
    offset: number;
};

export function getValue(data: ElfData[], field: string) {
    const bytes = data.find((d) => d.name == field)?.bytes || [];
    return bytesToNum(bytes);
}

const { hdr: elfHeader, hdrMap: elfHeaderMap } = parseElf2<EhdrFieldNames>(
    headerFieldsList.ehdr64,
);
const startOfProgramHeaders = getValue(elfHeader, "e_phoff");
const programHeaderSize = getValue(elfHeader, "e_phentsize");
const numberOfProgramHeaders = getValue(elfHeader, "e_phnum");
const startOfSectionHeaders = getValue(elfHeader, "e_shoff");
const sectionHeaderSize = getValue(elfHeader, "e_shentsize");
const numberOfSectionHeaders = getValue(elfHeader, "e_shnum");

const programHeaders = [];
const programHeaderMaps = [];
let phOffset = startOfProgramHeaders;
for (let i = 0; i < numberOfProgramHeaders; i++) {
    const { hdr, hdrMap } = parseElf2<PhdrFieldNames>(
        headerFieldsList.phdr64,
        phOffset,
    );
    programHeaders.push(hdr);
    programHeaderMaps.push(hdrMap);
    phOffset += programHeaderSize;
}
const sectionHeaders = [];
const sectionHeaderMaps = [];
let shoffset = startOfSectionHeaders;
for (let i = 0; i < numberOfSectionHeaders; i++) {
    const { hdr, hdrMap } = parseElf2<ShdrFieldNames>(
        headerFieldsList.shdr64,
        shoffset,
    );
    sectionHeaders.push(hdr);
    sectionHeaderMaps.push(hdrMap);
    shoffset += sectionHeaderSize;
}

// const elfHeaderMap: Record<string, number> = elfHeader.reduce((acc, curr) => {
//     acc[curr.name] = bytesToNum(curr.bytes);
//     return acc;
// }, {} as Record<string, number>);

// function arrToMap<T extends Record<K, number[]>, K extends string>(
// function arrToMap(
//     arr: ElfData[],
// ) {
//     const dataMap: Record<string, number> = {};
//     arr.forEach(({ name, bytes }) => dataMap[name] = bytesToNum(bytes));
//     return dataMap;
// }

// const elfHeaderMap: Record<string, number> = arrToMap(elfHeader);
// const elfHeaderMap: Record<string, number> = {};
// elfHeader.forEach(({ name, bytes }) => elfHeaderMap[name] = bytesToNum(bytes));

// type ProgramHeaderFields = headerFieldsList.phdr64

// const prkl = ["a", "b"] as const;
// type ProgramHeaderMap = Record<typeof prkl[number], string>;
// const prk: ProgramHeaderMap = {};

// const programHeaderMaps = programHeaders.map(arrToMap);

export const elfMeta = {
    elfHeader,
    elfHeaderMap,
    programHeaders,
    programHeaderMaps,
    sectionHeaders,
    sectionHeaderMaps,
    startOfProgramHeaders,
    numberOfProgramHeaders,
    programHeaderSize,
    startOfSectionHeaders,
    numberOfSectionHeaders,
    sectionHeaderSize,
};
