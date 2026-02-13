import { bytesToNum, type ElfData, elfMeta, getValue } from "./elfReader";

function formatBytes(bytes: number[]) {
    return "0x" + bytesToNum(bytes).toString(16);
}

export function ElfReader() {
    return (
        <div>
            <h1>ElfReader</h1>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr" }}>
                <KeyInformation />
                <ElfHeader />
            </div>
            <ProgramHeaders />
            <SectionHeaders />
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
            elfMeta.numberOfSectionHeaders,
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
    ) => getValue(phdr, "p_type") == 1).sort(byField("p_offset"))
        .map((phdr) => {
            const isExecutable = getValue(phdr, "p_flags") & 1;
            const p_offset = getValue(phdr, "p_offset");
            const p_vaddr = getValue(phdr, "p_vaddr");
            const e_entry = getValue(elfMeta.elfHeader, "e_entry");
            const realOffset = isExecutable
                ? p_offset + (e_entry - p_vaddr)
                : p_offset;
            return [
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
                const e_entry = getValue(elfMeta.elfHeader, "e_entry");
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
        <div>
            <h3>ELF header</h3>
            <table className="table">
                <tbody>
                    {elfMeta.elfHeader.map(({ bytes, name, offset }) => (
                        <tr key={offset}>
                            <td>{offset}</td>
                            <td>{name}</td>
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
        6: "Self-defining phdr",
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
            <SubHeaders
                // hdrs={elfMeta.programHeaders.sort(byField("p_type"))}
                hdrs={elfMeta.programHeaders}
                explType={(phdr: ElfData[]) =>
                    explPType(getValue(phdr, "p_type"))}
            />
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
            <SubHeaders
                hdrs={elfMeta.sectionHeaders.sort(byField("sh_offset"))}
                explType={(shdr: ElfData[]) =>
                    explShType(getValue(shdr, "sh_type"))}
            />
        </div>
    );
}

interface SubHeadersProps {
    hdrs: ElfData[][];
    explType: (hdr: ElfData[]) => string;
}

function SubHeaders({ hdrs, explType }: SubHeadersProps) {
    const fields = (hdrs?.[0] || []).map(({ name }) => name);
    return (
        <table className="table">
            <thead>
                <tr>
                    <th>p_type explained</th>
                    {fields.map((f) => <th key={f}>{f}</th>)}
                </tr>
            </thead>
            <tbody>
                {hdrs.map((hdr, i) => (
                    <tr key={i}>
                        <td>{explType(hdr)}</td>
                        {fields.map((f) => <td key={f}>{getValue(hdr, f)}</td>)}
                    </tr>
                ))}
            </tbody>
        </table>
    );
}
