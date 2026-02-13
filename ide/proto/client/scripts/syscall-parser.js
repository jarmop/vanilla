/**
 * Get file from:
 * https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl
 * 
 * Run with:
 * deno --allow-read syscall-parser.js
 */

const str = await Deno.readTextFile("syscall_64.tbl");

const rows = str.split('\n').filter(row => row[0] !== '#');

rows.forEach((row) => {
    const cols = row.split(/[\s\t]+/);
    if (cols[1] !== 'x32') {
        const cleanedRow = [cols[0], cols[2]].join('\t');
        console.log(cleanedRow);
    }
});
