import { bytesToNum, type ElfData, elfMeta, getValue } from "./elfReader";

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
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr" }}>
                <KeyInformation />
                <ElfHeader />
            </div>
            <ProgramHeaders />
            <SectionHeaders />
            <hr />
        </div>
    );
}

function KeyInformation() {
    const loadableSegments = elfMeta.programHeaders.filter((phdr) =>
        getValue(phdr, "p_type") == 1
    );
    const data = [
        [
            "Start of program headers",
            elfMeta.startOfProgramHeaders.toString(16),
        ],
        ["Number of program headers", elfMeta.numberOfProgramHeaders],
        ["Total size of program headers", elfMeta.programHeaderSize],
        [
            "Start of section headers",
            elfMeta.startOfSectionHeaders.toString(16),
        ],
        [
            "Number of section headers",
            elfMeta.numberOfSectionHeaders.toString(16),
        ],
        [
            "Total size of section headers",
            elfMeta.numberOfSectionHeaders *
            elfMeta.sectionHeaderSize,
        ],
        ["Number of loadable segments", loadableSegments.length],
        [
            "Total size of loadable segments",
            loadableSegments.reduce(
                (size, shdr) => size + getValue(shdr, "p_filesz"),
                0,
            ),
        ],
    ];

    const phdrOffsetInfo: [number, number][] = elfMeta.programHeaders.filter((
        phdr,
    ) => getValue(phdr, "p_type") == 1 // && getValue(phdr, "p_filesz") > 0
    ).sort(byField("p_offset"))
        .map((phdr) => {
            const isExecutable = getValue(phdr, "p_flags") & 1;
            const p_offset = getValue(phdr, "p_offset");
            const p_vaddr = getValue(phdr, "p_vaddr");
            const e_entry = getValue(elfMeta.ehdr, "e_entry");
            const realOffset = isExecutable
                ? p_offset + (e_entry - p_vaddr)
                : p_offset;
            return [
                // e_entry + p_offset,
                realOffset,
                getValue(phdr, "p_filesz"),
            ];
        });

    const shdrOffsetInfo: [number, number][] = elfMeta.sectionHeaders.filter((
        shdr,
    ) => getValue(shdr, "sh_size") > 0).sort(byField("sh_offset"))
        .map((shdr) => {
            return [
                getValue(shdr, "sh_offset"),
                getValue(shdr, "sh_size"),
            ];
        });

    const executableSegments = elfMeta.programHeaders.filter((phdr) =>
        getValue(phdr, "p_flags") & 1
    );

    return (
        <div>
            <h3>Key information</h3>
            <table className="table">
                <tbody>
                    {data.map(([label, value]) => (
                        <tr key={label}>
                            <td>{label}</td>
                            <td>{value}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
            <div style={{ margin: "10px 0 4px 0" }}>
                Executable segment offsets:
            </div>
            {executableSegments.map((phdr) => {
                const p_offset = getValue(phdr, "p_offset");
                const p_vaddr = getValue(phdr, "p_vaddr");
                const e_entry = getValue(elfMeta.ehdr, "e_entry");
                const realOffset = p_offset + e_entry - p_vaddr;
                return (
                    <div key={p_offset}>
                        {formatBytes([realOffset])}
                    </div>
                );
            })}

            <div style={{ margin: "10px 0 4px 0" }}>
                Loadable segment (code) offsets
            </div>
            <OffsetGrid offsetInfo={phdrOffsetInfo} />

            <div style={{ margin: "10px 0 4px 0" }}>
                Section offsets
            </div>
            <OffsetGrid
                offsetInfo={shdrOffsetInfo}
            />
        </div>
    );
}

function OffsetGrid({ offsetInfo }: { offsetInfo: [number, number][] }) {
    return (
        <div
            style={{
                display: "flex",
                flexWrap: "wrap",
                // display: "grid",
                // gridTemplateColumns: "repeat(auto-fill, 120px)",
                // gap: "5px",
            }}
        >
            {offsetInfo.map(
                ([offset, size], i) => {
                    return (
                        <div
                            key={i}
                            style={{
                                border: "1px solid black",
                                whiteSpace: "nowrap",
                                padding: "5px",
                                margin: "3px",
                                fontSize: "14px",
                                // width: "100px",
                            }}
                        >
                            {bytesToNum([offset])} -{" "}
                            {bytesToNum([offset + size])}
                        </div>
                    );
                },
            )}
        </div>
    );
}

function ElfHeader() {
    return (
        <div
            // style={{
            //     borderRight: "1px solid black",
            //     marginRight: "10px",
            //     paddingRight: "10px",
            // }}
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
    );
}

function byField(field: string) {
    return (a: ElfData[], b: ElfData[]) =>
        getValue(a, field) - getValue(b, field);
}

function explPType(pType: number) {
    const pTypeMap: Record<number, string> = {
        0: "Unused",
        1: "Loadable segment",
        2: "Dynamic linking information",
        3: "Interpreter information",
        4: "Auxiliary information",
        5: "Reserved",
        6: "Contains phdr table",
        7: "Thread-Local Storage template.",
    };
    const expl = pTypeMap[pType];
    if (expl) {
        return expl;
    } else if (pType > 0x60000000 && pType < 0x6FFFFFFF) {
        return "OS specific";
    } else if (pType > 0x70000000 && pType < 0x7FFFFFFF) {
        return "Processor specific";
    }
    return "?";
}

function ProgramHeaders() {
    return (
        <div>
            <h3>Program headers</h3>
            <div>
                Total size: {elfMeta.programHeaders.reduce((acc, phdr) => {
                    return acc + getValue(phdr, "p_filesz");
                }, 0)} bytes
            </div>
            <div
                style={{
                    display: "grid",
                    gridTemplateColumns: "repeat(6, 1fr)",
                    gap: "10px",
                    // background: "#c7c7c7",
                    // background: "#eee",
                }}
            >
                {elfMeta.programHeaders.sort(byField("p_type")).map((
                    phdr,
                    i,
                ) => (
                    <div
                        key={i}
                        style={{
                            padding: "4px",
                            background: "#f7f7f7",
                            border: "1px solid #c7c7c7",
                        }}
                    >
                        <div style={{ marginBottom: "4px" }}>
                            {explPType(getValue(phdr, "p_type"))}
                        </div>
                        <table
                            key={i}
                            className="table"
                            style={{
                                background: "white",
                                width: "100%",
                            }}
                        >
                            <tbody>
                                {phdr.map(({ bytes, name, offset }) => (
                                    <tr key={offset}>
                                        <td>{offset}</td>
                                        <td>{name}</td>
                                        {/* <td>{formatBytes(bytes)}</td> */}
                                        <td>{bytesToNum(bytes)}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    </div>
                ))}
            </div>
        </div>
    );
}

function explShType(shType: number) {
    const shTypeMap: Record<number, string> = {
        0: "Unused",
        1: "Program data",
        2: "Symbol table",
        3: "String table",
        4: "Relocation entries with addends",
        5: "Symbol hash table",
        6: "Dynamic linking information",
        7: "Notes",
        8: "BSS",
        9: "Relocation entries, no addends",
        10: "Reserved",
        11: "Dynamic linker symbol table",
        14: "Array of constructors",
        15: "Array of destructors",
        16: "Array of pre-constructors",
        17: "Section group",
        18: "Extended section indices",
        19: "Number of defined types.",
    };
    const expl = shTypeMap[shType];
    if (expl) {
        return expl;
    } else if (shType > 0x60000000) {
        return "OS specific";
    }
    return "?";
}

function SectionHeaders() {
    return (
        <div>
            <h3>Section headers</h3>
            {/* <TypeCount /> */}
            <div>
                Total size: {elfMeta.sectionHeaders.reduce((acc, shdr) => {
                    return acc + getValue(shdr, "sh_size");
                }, 0)} bytes
            </div>
            <div
                style={{
                    display: "grid",
                    gridTemplateColumns: "repeat(6, 1fr)",
                    gap: "10px",
                    // background: "#c7c7c7",
                    // background: "#eee",
                }}
            >
                {elfMeta.sectionHeaders.sort(byField("sh_offset")).map((
                    shdr,
                    i,
                ) => (
                    <div
                        key={i}
                        style={{
                            padding: "4px",
                            background: "#f7f7f7",
                            border: "1px solid #c7c7c7",
                        }}
                    >
                        <div style={{ marginBottom: "4px" }}>
                            {explShType(getValue(shdr, "sh_type"))}
                        </div>
                        <table
                            key={i}
                            className="table"
                            style={{
                                background: "white",
                                width: "100%",
                            }}
                        >
                            <tbody>
                                {shdr.map(({ bytes, name, offset }) => (
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
                ))}
            </div>
        </div>
    );
}

function TypeCount() {
    const typeCounts: Record<number, number> = {};
    elfMeta.sectionHeaders.forEach((shdr) => {
        const type = getValue(shdr, "sh_type");
        if (!typeCounts[type]) {
            typeCounts[type] = 0;
        }
        typeCounts[type]++;
    });

    return (
        <table>
            <tbody>
                {Object
                    .entries(typeCounts)
                    .sort(([_a, a], [_b, b]) => b - a) // sort by size
                    .map(([k, v]) => (
                        <tr>
                            <td>{explShType(parseInt(k))}</td>
                            <td>{v}</td>
                        </tr>
                    ))}
            </tbody>
        </table>
    );
}
