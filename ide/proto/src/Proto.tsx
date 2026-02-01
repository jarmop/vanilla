import { Fragment, type ReactNode } from "react";
import "./App.css";
import { bytes } from "./bin";
import { leftPad } from "./helper";

const rowLength = 8;
const borderColor = "#222";
const backgroundColor = "#fcfcfc";
const elf = {
    ehdr_bytes: 64,
    phdr_bytes: 56,
    code_bytes: 34,
};

function isAsciiText(byte: number) {
    // const isValidChar = (byte > 47 && byte < 58) || (byte >= 64 && byte <= 126);
    return byte > 31 && byte <= 126;
}

const byteRows: number[][] = [];
for (let i = 0; i < bytes.length; i += rowLength) {
    byteRows.push(bytes.slice(i, i + rowLength));
}

export function Proto() {
    return (
        <>
            <h1>The original proto</h1>
            <div style={{ display: "flex", flexDirection: "row" }}>
                <AddrCol byteRows={byteRows} />
                {/* <DecCol /> */}
                {/* <BinCol /> */}
                <HexCol />
                <ElfLayoutCol />
                <AsciiCol />
            </div>
        </>
    );
}

function AddrCol({ byteRows }: { byteRows: number[][] }) {
    let offset = 0;
    return (
        <div className="window">
            {byteRows.map((row, i) => {
                const addr = offset;
                offset += row.length;
                return <div key={i}>{leftPad(addr.toString(16), 16)}</div>;
            })}
        </div>
    );
}

function ByteToHex({ byte }: { byte: number }) {
    const color = byte > 0 ? isAsciiText(byte) ? "#0a0" : "#a00" : "#c7c7c7";
    return (
        <span
            style={{ color, marginRight: "4px" }}
        >
            {leftPad(byte.toString(16), 2)}
        </span>
    );
}

function HexCol() {
    const rows: ReactNode[] = [];
    for (let i = 0; i < bytes.length; i += rowLength) {
        rows.push(
            <Fragment key={i}>
                {bytes
                    .slice(i, i + rowLength)
                    .map((byte, j) => (
                        <ByteToHex key={`${i}-${j}`} byte={byte} />
                    ))}
            </Fragment>,
        );
    }

    return (
        <div className="window" style={{ borderColor }}>
            {rows.map((row, i) => (
                <div key={i}>
                    {row}
                </div>
            ))}
        </div>
    );
}

function AsciiCol() {
    function byteToAscii(byte: number) {
        const ascii = isAsciiText(byte) ? String.fromCharCode(byte) : ".";
        return ascii;
    }
    const rows: (string | ReactNode)[] = [];
    for (let i = 0; i < bytes.length; i += rowLength) {
        const row = bytes
            .slice(i, i + rowLength)
            .map(byteToAscii);
        rows.push(row);
    }

    return (
        <div className="window" style={{ borderColor }}>
            {rows.map((row, i) => (
                <div key={i}>
                    {row}
                </div>
            ))}
        </div>
    );
}

interface BinColProps {
}

function byteToBin(byte: number) {
    return leftPad(byte.toString(2), 8);
}

function BinCol({}: BinColProps) {
    const bit0 = "#999";
    const bit1 = "#555";

    const rows: ReactNode[] = [];
    for (let i = 0; i < bytes.length; i += rowLength) {
        const row = bytes
            .slice(i, i + rowLength)
            .map(byteToBin);
        // .join(delimiter);
        // rows.push(row);
        rows.push(
            <div
                style={{
                    display: "flex",
                    borderTop: `2px solid ${backgroundColor}`,
                    borderBottom: `2px solid ${backgroundColor}`,
                    boxSizing: "border-box",
                    height: "15px",
                }}
            >
                {row.map((byte, i) => (
                    <div
                        key={i}
                        style={{
                            display: "flex",
                            borderTop: `1px solid ${borderColor}`,
                            borderBottom: `1px solid ${borderColor}`,
                            borderLeft: `1px solid ${borderColor}`,
                            boxSizing: "border-box",
                            marginRight: "6px",
                        }}
                    >
                        {byte.split("").map((bit, i) => {
                            return (
                                <div
                                    key={i}
                                    style={{
                                        padding: "2px",
                                        boxSizing: "border-box",
                                        width: "6px",
                                        borderRight: `1px solid ${borderColor}`,
                                        background: parseInt(bit) ? bit1 : bit0,
                                    }}
                                >
                                </div>
                            );
                        })}
                    </div>
                ))}
            </div>,
        );
    }

    return (
        <div className="window" style={{ borderColor }}>
            {rows.map((row, i) => (
                <div
                    key={i}
                    // style={{ height: "15px" }}
                >
                    {row}
                </div>
            ))}
        </div>
    );
}

function DecCol() {
    function byteToDec(byte: number) {
        return leftPad(byte.toString(), 3);
    }
    const rows: (string | ReactNode)[] = [];
    for (let i = 0; i < bytes.length; i += rowLength) {
        const row = bytes
            .slice(i, i + rowLength)
            .map(byteToDec)
            .join("|");
        rows.push(row);
    }

    return (
        <div className="window" style={{ borderColor }}>
            {rows.map((row, i) => (
                <div key={i}>
                    {row}
                </div>
            ))}
        </div>
    );
}

function ElfLayoutCol() {
    const rpx = 15; // pixels per 8 bytes
    const ehdrHeight = elf.ehdr_bytes / 8 * rpx + "px";
    const phdrHeight = Math.ceil(elf.phdr_bytes / 8) * rpx + "px";
    const codeHeight = Math.ceil(elf.code_bytes / 8) * rpx + "px";

    return (
        <div
            style={{
                padding: "10px 0",
                fontSize: "12px",
                fontFamily: "arial",
            }}
        >
            <div
                style={{
                    boxSizing: "border-box",
                    height: ehdrHeight,
                    border: "1px solid black",
                    borderWidth: "1px 0 0 0 ",
                }}
            >
                ELF header
            </div>
            {/* <div style={{ height: "60px" }}>?</div> */}
            <div
                style={{
                    boxSizing: "border-box",
                    height: phdrHeight,
                    border: "1px solid black",
                    borderWidth: "1px 0 0 0",
                }}
            >
                Program header
            </div>
            {/* <div style={{ height: "45px" }}>?</div> */}
            <div
                style={{
                    boxSizing: "border-box",
                    height: codeHeight,
                    border: "1px solid black",
                    borderWidth: "1px 0 ",
                }}
            >
                Instructions
            </div>
        </div>
    );
}
