// 15 bytes at most per instruction, so no instruction ever fully
// fills up all the fields (which would add up to 17 bytes)
const headers = [
    { name: "Prefixes", size: 4 },
    { name: "Op", size: 3 },
    { name: "ModR/M", size: 1 },
    { name: "SIB", size: 1 },
    { name: "Displacement", size: 4 },
    { name: "Immediate", size: 4 },
];

// const descriptions = [""];

const instr = [{
    prefix: ["", "", "", ""],
    opcode: ["B8", "", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["01", "00", "00", "00"],
    description: [
        'mov eax,1  –  Put the linux syscall id for "write" (1) into the eax register',
    ],
}, {
    prefix: ["", "", "", ""],
    opcode: ["BF", "", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["01", "00", "00", "00"],
    description: [
        "mov edi,1  –  Put the file id into the edi register (1 = stdout)",
    ],
}, {
    prefix: ["48", "", "", ""],
    opcode: ["8D", "", ""],
    modrm: ["35"],
    sib: [""],
    displacement: ["10", "00", "00", "00"],
    immediate: ["", "", "", ""],
    description: [
        `lea rsi,[rip+0x10]  –  Put the address of the string into the rsi register
         (distance from the current instruction to the string is 16 bytes)
         0x8D is the op code for lea. 0x48 is a prefix telling that the operands are 64 bits.
         0x35 is the ModR/M byte for *SI destination register and 32-bit displacement 
         (Table 2-2 in chapter 2.1.3 of the x86 instruction reference manual)`,
    ],
}, {
    prefix: ["", "", "", ""],
    opcode: ["BA", "", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["0F", "00", "00", "00"],
    description: [
        `mov edx,15  –  Put the size of the string into the edx register 
        "Hello, World!\n" is 15 bytes, "\n" is translated by linux into  
        two separate characters: line feed, and carriage return`,
    ],
}, {
    prefix: ["", "", "", ""],
    opcode: ["0F", "05", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["", "", "", ""],
    description: [""],
}, {
    prefix: ["", "", "", ""],
    opcode: ["B8", "", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["3C", "00", "00", "00"],
    description: [""],
}, {
    prefix: ["", "", "", ""],
    opcode: ["31", "", ""],
    modrm: ["FF"],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["", "", "", ""],
    description: [""],
}, {
    prefix: ["", "", "", ""],
    opcode: ["0F", "05", ""],
    modrm: [""],
    sib: [""],
    displacement: ["", "", "", ""],
    immediate: ["", "", "", ""],
    description: [""],
}];

const instructions: string[][][] = instr.map((o) =>
    Object.values(o).slice(0, -1)
);

// const instructions: number[][] = [
//     [0, 0, 0, 0, 0, 0, 0xB8, 0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x00],
//     [0, 0, 0, 0, 0, 0, 0xBF, 0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x00],
//     [0, 0, 0, 0, 0, 0, 0xBF, 0, 0, 0, 0, 0, 0, 0x01, 0x00, 0x00, 0x00],
// ];

const firstColStyles = {
    borderLeftColor: "#777",
    borderLeftWidth: "2px",
};

export function Assembler() {
    return (
        <>
            <h1>Assembler</h1>
            <table
                style={{
                    borderCollapse: "collapse",
                }}
                className="AssemblerTable"
            >
                <thead>
                    <tr>
                        {headers.map(({ name, size }) => (
                            <th
                                title={name}
                                key={name}
                                colSpan={size}
                                style={{
                                    // overflow: "hidden",
                                    // maxWidth: size * 10 + "px",
                                }}
                            >
                                {name}
                            </th>
                        ))}
                        <th>Description</th>
                    </tr>
                </thead>
                <tbody>
                    {instructions.map((ins, i) => (
                        <tr key={i}>
                            {ins.map((sect) =>
                                sect.map((byte, i) => (
                                    <td
                                        key={`${headers[i]} ${i}`}
                                        style={{
                                            width: "20px",
                                            // textTransform: "uppercase",
                                            textAlign: "center",
                                            ...(i == 0 ? firstColStyles : {}),
                                        }}
                                    >
                                        {byte}
                                    </td>
                                ))
                            )}
                            <td style={firstColStyles}>
                                {instr[i].description}
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </>
    );
}
