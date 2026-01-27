// deno run --allow-net your_script.ts
import { DOMParser } from "https://deno.land/x/deno_dom/deno-dom-wasm.ts";

function firstRowValuesPerTbody(html) {
  const doc = new DOMParser().parseFromString(html, "text/html");
  if (!doc) throw new Error("Failed to parse HTML");

  const tbodies = doc.querySelectorAll("tbody");
  const result = [];

  for (const tbody of tbodies) {
    const firstTr = tbody.querySelector("tr");
    if (!firstTr) {
      result.push([]);
      continue;
    }

    const tds = firstTr.querySelectorAll("td");
    const row = [];
    for (const td of tds) {
      // textContent strips <b>, <i>, etc.
      const text = (td.textContent ?? "")
        .replace(/\s+/g, " ")
        .trim();
      row.push(text);
    }
    result.push(row);
  }

  return result;
}

const html = await Deno.readTextFile("x86-two-byte-op.html");

const rows = firstRowValuesPerTbody(html);

// const fou = {};

// console.log(rows.every(r => r.length === 22));

const table = [];

rows.forEach(r => {
    // if (fou[r.length] === undefined) {
    //     fou[r.length] = 0;
    // }
    // fou[r.length]++;
    // if (r.length === 18) {
    //     console.log(r[r.length -1]);
    //     // console.log(r);
    // }
    if (r.length === 22) {
        table.push(r);
    }
})

// console.log(fou);
console.log(JSON.stringify(table));