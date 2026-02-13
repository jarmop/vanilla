const text = await Deno.readTextFile("x86-two-byte-op.html");

// const obj: Record<string, string> = {};

// const foo = {
//     "Prefix": "",
//     "Prefix 0F": "",
//     "Primary Opcode": "",
//     "Secondary Opcode": "",
//     "Register/Opcode Field": "",
//     "Introduced with the Processor": "",
//     "Documentation Status": "",
//     "Mode of Operation": "",
//     "Ring Level": "",
//     "Lock Prefix/FPU Push/FPU Pop": "",
//     "Instruction Mnemonic": "",
//     "Operand 1": "",
//     "Operand 2": "",
//     "Operand 3": "",
//     "Operand 4": "",
//     "Instruction Extension Group": "",
//     "Tested Flags": "",
//     "Modified Flags": "",
//     "Defined Flags": "",
//     "Undefined Flags": "",
//     "Flags Values": "",
//     "Description, Notes": "",
// };

// type Row = {
//     "Prefix": string;
//     "Prefix 0F": string;
//     "Primary Opcode": string;
//     "Secondary Opcode": string;
//     "Register/Opcode Field": string;
//     "Introduced with the Processor": string;
//     "Documentation Status": string;
//     "Mode of Operation": string;
//     "Ring Level": string;
//     "Lock Prefix/FPU Push/FPU Pop": string;
//     "Instruction Mnemonic": string;
//     "Operand 1": string;
//     "Operand 2": string;
//     "Operand 3": string;
//     "Operand 4": string;
//     "Instruction Extension Group": string;
//     "Tested Flags": string;
//     "Modified Flags": string;
//     "Defined Flags": string;
//     "Undefined Flags": string;
//     "Flags Values": string;
//     "Description, Notes": string;
// };

let rowCount = 0;
let colCount = 0;

const rowStarted = false;

// const titles: string[] = [];
const titles = [
    "Prefix",
    "Prefix 0F",
    "Primary Opcode",
    "Secondary Opcode",
    "Register/Opcode Field",
    "Introduced with the Processor",
    "Documentation Status",
    "Mode of Operation",
    "Ring Level",
    "Lock Prefix/FPU Push/FPU Pop",
    "Instruction Mnemonic",
    "Operand 1",
    "Operand 2",
    "Operand 3",
    "Operand 4",
    "Instruction Extension Group",
    "Tested Flags",
    "Modified Flags",
    "Defined Flags",
    "Undefined Flags",
    "Flags Values",
    "Description, Notes",
] as const;

type Title = typeof titles[number];

type Row = Record<Title, string>;

// const foo = ['','0F','00','','0','','','P','','','SLDT','m16','LDTR','','','','','','','','','Store Local Descriptor Table Register'];
const fur = [
    "",
    "0F",
    "00",
    "",
    "0",
    "",
    "",
    "P",
    "",
    "",
    "SLDT",
    "m16",
    "LDTR",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Store Local Descriptor Table Register",
];

console.log(fur.length);

const table: Row[] = [];

// text.split("\n").forEach((row) => {
//     // if (row.startsWith("<th title=")) {
//     //     const title = row.split('"')[1];
//     //     // console.log(`"${title}": string,`);
//     //     titles.push(`"${title}"`);
//     // }
//     if (row.startsWith("<tbody id=")) {
//         const cols = row.split("<td>");
//         colCount += cols.length;
//     }
// });
let foo: string = "";
// const rows = text.split("<tr>").slice(2).map((r) => r.split("</tr>")[0]);
const re = /<tr(.*?)>/;
const rows = text.split(re).slice(2).map((r) => r.split("</tr>")[0]);
foo = rows[0];
// rows.forEach((row) => {
//     const cols = row.split("<td>").map;
// });

// console.log(foo);

// console.log(titles.join(","));
