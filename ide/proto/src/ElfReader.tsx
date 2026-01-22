import { ehdr, elfData2, phdr } from "./elfReader";
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
    return bytes
        .map((b) => leftPad(b.toString(16), 2))
        .join(" ");
}

export function ElfReader() {
    return (
        <div>
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
                            {/* {elfDataList.map(({ bytes, name, offset }) => ( */}
                            {ehdr.map(({ bytes, name, offset }) => (
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
                <div>
                    <h3>Program header</h3>
                    <table className="table">
                        <tbody>
                            {/* {elfDataList.map(({ bytes, name, offset }) => ( */}
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
                </div>
            </div>
            <hr />
            <table>
                <tbody>
                    {Object.entries(elfData2).map(([k, v]) => (
                        <tr key={k}>
                            <td>{k}:</td>
                            <td>{v}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
}
