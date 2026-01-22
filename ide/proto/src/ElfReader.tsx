import { bytesToNum, elfMeta } from "./elfReader";
import { leftPad } from "./helper";

// type ElfField = {
//     offset: number; // bytes
//     size: number; // bytes
// };

// const ELF = { ehdr: [{}] };

// const ELF_MAGIC = [127, 69, 76, 70];

// function isElf(bytes: number[]) {
//     return bytes.slice(0, 4).every((byte, i) => byte === ELF_MAGIC[i]);
// }

function formatBytes(bytes: number[]) {
    return "0x" + bytesToNum(bytes).toString(16);
    // return bytes
    //     .map((b) => leftPad(b.toString(16), 2))
    //     .join(" ");
}

export function ElfReader() {
    return (
        <div>
            <table className="table">
                <tbody>
                    <tr>
                        <td>Start of program headers:</td>
                        <td>{elfMeta.startOfProgramHeaders.toString(16)}</td>
                    </tr>
                    <tr>
                        <td>Number of program headers:</td>
                        <td>{elfMeta.numberOfProgramHeaders}</td>
                    </tr>
                    <tr>
                        <td>Start of section headers:</td>
                        <td>{elfMeta.startOfSectionHeaders.toString(16)}</td>
                    </tr>
                    <tr>
                        <td>Number of section headers:</td>
                        <td>{elfMeta.numberOfSectionHeaders}</td>
                    </tr>
                    <tr>
                        <td>Start of instructions:</td>
                        <td>{elfMeta.codeOffset.toString(16)}</td>
                    </tr>
                    <tr>
                        <td>Size of instructions:</td>
                        <td>{elfMeta.codeSize}</td>
                    </tr>
                </tbody>
            </table>
            <div style={{ display: "flex" }}>
                <div
                    style={{
                        borderRight: "1px solid black",
                        marginRight: "10px",
                        paddingRight: "10px",
                    }}
                >
                    <h3>ELF header</h3>
                    <table className="table">
                        <tbody>
                            {elfMeta.ehdr.map(({ bytes, name, offset }) => (
                                <tr key={offset}>
                                    <td>{offset}</td>
                                    <td>{name}</td>
                                    {/* <td>{formatBytes(bytes)}</td> */}
                                    <td>{formatBytes(bytes)}</td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
                <div style={{ display: "grid" }}>
                    <h3>Program headers</h3>
                    <div>
                        {elfMeta.programHeaders.map((phdr, i) => (
                            <table key={i} className="table">
                                <tbody>
                                    {phdr.map(({ bytes, name, offset }) => (
                                        <tr key={offset}>
                                            <td>{offset}</td>
                                            <td>{name}</td>
                                            {/* <td>{formatBytes(bytes)}</td> */}
                                            <td>{formatBytes(bytes)}</td>
                                        </tr>
                                    ))}
                                </tbody>
                            </table>
                        ))}
                    </div>
                </div>
            </div>
            <hr />
            <ByteTable />
            <hr />
        </div>
    );
}

function ByteTable() {
    return (
        <table>
            <tbody>
                <tr>
                    <td>Instructions</td>
                    <td>Bytes</td>
                </tr>
            </tbody>
        </table>
    );
}
