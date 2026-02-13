import { Fragment } from "react/jsx-runtime";
import { bytes } from "./bin";
import { elfMeta, getValue } from "./elfReader";
import { leftPad } from "./helper";

const rowLength = 32;

const {
    elfHeaderMap: elfHeader,
    programHeaderMaps: programHeaders,
    sectionHeaderMaps: sectionHeaders,
} = elfMeta;

function getCodeEntry() {
    const codeHdr = elfMeta.programHeaders.find((hdr) =>
        getValue(hdr, "p_flags") & 1
    );
    if (!codeHdr) {
        return;
    }
    const p_offset = getValue(codeHdr, "p_offset");
    const e_entry = getValue(elfMeta.elfHeader, "e_entry");
    const p_vaddr = getValue(codeHdr, "p_vaddr");

    return ({
        name: "Code entry",
        offset: p_offset + (e_entry - p_vaddr),
        size: getValue(codeHdr, "p_filesz"),
    });
}

type ByteBlock = { name: string; offset: number; size: number };

export function BytesViewTable() {
    const codeEntryBlock = getCodeEntry();
    if (!codeEntryBlock) {
        return;
    }

    const ub1_start = elfHeader.e_phoff +
        elfHeader.e_phnum * elfHeader.e_phentsize;
    const unknownBlock1 = {
        name: "?",
        offset: ub1_start,
        size: codeEntryBlock.offset - ub1_start,
    };

    const ub2_start = codeEntryBlock.offset + codeEntryBlock.size;
    const unknownBlock2 = {
        name: "?",
        offset: ub2_start,
        size: elfHeader.e_shoff - ub2_start,
    };

    const byteBlocks = [
        {
            name: "ELF header",
            offset: 0,
            size: 64,
        },
        ...programHeaders.map((_, i) => {
            return {
                name: `Program header ${i + 1}`,
                offset: elfHeader.e_phoff + i * elfHeader.e_phentsize,
                size: elfHeader.e_phentsize,
            };
        }),
        unknownBlock1,
        codeEntryBlock,
        unknownBlock2,
        ...sectionHeaders.map((_, i) => {
            return {
                name: `Section header ${i + 1}`,
                offset: elfHeader.e_shoff + i * elfHeader.e_shentsize,
                size: elfHeader.e_shentsize,
            };
        }),
    ];

    return (
        <>
            <h1>BytesViewTable</h1>
            <table className="BytesViewTable">
                {byteBlocks.map((bb, i) => (
                    <Fragment key={i}>
                        {i > 0 && <ByteBlockDivider />}
                        <ByteBlock
                            byteBlock={bb}
                        />
                    </Fragment>
                ))}
            </table>
        </>
    );
}

interface ByteBlockProps {
    byteBlock: ByteBlock;
}

function ByteBlock({ byteBlock }: ByteBlockProps) {
    const { name, offset, size } = byteBlock;
    const blockBytes = bytes.slice(offset, offset + size);
    const rows: [number, number[]][] = [];
    for (let i = 0; i < blockBytes.length; i += rowLength) {
        rows.push([offset + i, blockBytes.slice(i, i + rowLength)]);
    }

    return (
        <tbody>
            {rows.map(([address, rowBytes], i) => {
                return (
                    <tr key={i}>
                        {i == 0 && <td rowSpan={rows.length}>{name}</td>}
                        <td className="mono">{address}</td>
                        <td className="mono">
                            {rowBytes.map((b) => leftPad(b.toString(16), 2))
                                .join(
                                    " ",
                                )}
                        </td>
                    </tr>
                );
            })}
        </tbody>
    );
}

function ByteBlockDivider() {
    return (
        <tbody className="BytesDivider">
            <tr>
                <td
                    style={{ height: "10px" }}
                >
                    <hr />
                </td>
                <td>
                    <hr />
                </td>
                <td>
                    <hr />
                </td>
            </tr>
        </tbody>
    );
}
