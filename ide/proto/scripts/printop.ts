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

const keytitles: Title[] = [
    "Prefix",
    "Prefix 0F",
    "Primary Opcode",
    "Secondary Opcode",
    "Register/Opcode Field",
    "Instruction Mnemonic",
    "Operand 1",
    "Operand 2",
    "Operand 3",
    "Operand 4",
    "Description, Notes",
];

type Title = typeof titles[number];

type Row = Record<Title, string>;

function getRows(arr: string[][]) {
    const rows: Row[] = [];
    arr.filter((v) => v[2].length > 0).forEach((values) => {
        const row: Row = titles.reduce((row, title, i) => {
            row[title] = values[i];
            return row;
        }, {} as Row);
        rows.push(row);
    });
    return rows;
}

const json1 = await Deno.readTextFile("x86-one-byte-op.json");
const json2 = await Deno.readTextFile("x86-two-byte-op.json");
const arr1: string[][] = JSON.parse(json1);
const arr2: string[][] = JSON.parse(json2);
const rows1: Row[] = getRows(arr1);
const rows2: Row[] = getRows(arr2);

function printOp(op: string, rows: Row[]) {
    const oprow = rows.find((r) => r["Primary Opcode"] === op);
    oprow && keytitles.forEach((k) => {
        let pad = "";
        for (let i = 0; i < 21 - k.length; i++) {
            pad += " ";
        }
        console.log(pad + k + ": " + (oprow[k].length > 0 ? oprow[k] : "-"));
    });
}

printOp("B8", rows1);
console.log("\n");
printOp("31", rows1);
console.log("\n");
printOp("05", rows2);
