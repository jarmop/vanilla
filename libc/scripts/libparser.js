/**
 * Get file from:
 * http://www.schweikhardt.net/identifiers.html
 * 
 * Run with:
 * deno --allow-read libparser.js sys/socket posix
 */

const lib = Deno.args[0];
const spec = Deno.args[1];

const str = spec === 'posix' ? await Deno.readTextFile("posix.tbl") 
    : await Deno.readTextFile("std.tbl");

const rows = str.split('\n').filter(row => row[0] !== '#');

const bystd = {};

rows.forEach((row) => {
    const cols = row.split(/[\s\t]+/);
    const [name, header, standard] = cols;

    if (header === `${lib}.h`) {
        if (!bystd[standard]) {
            bystd[standard] = [];
        }
        bystd[standard].push(name);
        // console.log(`${name}\t\t${standard}`);
    }
});

// console.log(Object.keys(bystd));
// console.log('C89', bystd['C89']);
// console.log(bystd['C99']);
// console.log(bystd['C11']);

Object.entries(bystd).forEach(([std, names]) => {
    console.log(std + ':');
    console.log(names.join(', '));
    // console.log(names.map(n => `${n}\t${std}`).join('\n'));
})
