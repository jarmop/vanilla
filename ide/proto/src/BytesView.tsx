import { Fragment } from "react/jsx-runtime";
import { bytes } from "./bin";
import { elfMeta, getValue } from "./elfReader";
import { leftPad } from "./helper";

export function BytesView() {
    return (
        <div
            className="BytesView"
            style={{
                display: "grid",
                gridTemplateColumns: "repeat(3, max-content)",
                gap: "0 10px",
            }}
        >
            <ElfHeaderBlock />
            <CodeBytesBlock startRow={3} />
        </div>
    );
}

function ElfHeaderBlock() {
    const offset = 0;
    const size = 64;
    const blockBytes = bytes.slice(0, size);
    const rows = [];
    const rowLength = 32;
    for (let i = 0; i < blockBytes.length; i += rowLength) {
        rows.push(blockBytes.slice(i, i + rowLength));
    }

    let address = offset;
    return (
        <>
            <div style={{ gridRow: `1 / ${rows.length + 1}` }}>
                ELF header
            </div>
            <>
                {rows.map((r) => {
                    const tmpAddress = address;
                    address += rowLength;
                    return (
                        <Fragment key={tmpAddress}>
                            <div>{tmpAddress}</div>
                            <div style={{ fontFamily: "monospace" }}>
                                {r.map((b) => leftPad(b.toString(16), 2))
                                    .join(
                                        " ",
                                    )}
                            </div>
                        </Fragment>
                    );
                })}
            </>
        </>
    );
}

interface CodeBytesBlock {
    startRow: number;
}

function CodeBytesBlock({ startRow }: CodeBytesBlock) {
    const codeOffset = getValue(elfMeta.elfHeader, "e_entry");
    const codeHdr = elfMeta.programHeaders.find((hdr) =>
        getValue(hdr, "p_flags") & 1
    );
    if (!codeHdr) {
        return;
    }
    const codeSize = getValue(codeHdr, "p_filesz");
    const codeBytes = bytes.slice(codeOffset, codeOffset + codeSize);
    const rows = [];
    const rowLength = 32;
    for (let i = 0; i < codeBytes.length; i += rowLength) {
        rows.push(codeBytes.slice(i, i + rowLength));
    }

    let address = codeOffset;
    return (
        <>
            <div
                style={{
                    gridRow: `${startRow} / ${rows.length + startRow + 1}`,
                }}
            >
                Code entry
            </div>
            <>
                {rows.map((r) => {
                    const tmpAddress = address;
                    address += rowLength;
                    return (
                        <Fragment key={tmpAddress}>
                            <div>{tmpAddress}</div>
                            <div style={{ fontFamily: "monospace" }}>
                                {r.map((b) => leftPad(b.toString(16), 2))
                                    .join(
                                        " ",
                                    )}
                            </div>
                        </Fragment>
                    );
                })}
            </>
        </>
    );
}
