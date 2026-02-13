import { code } from "../../data/codestrings/ide.js";
import { FuncHdr } from "./FuncHdr.tsx";

// const BORDER = "1px solid #666";

export function ProgramC() {
    return (
        <div
            className="ProgramEditor"
            style={{ width: "100%", height: "100%" }}
        >
            <div style={{ width: "fit-content", padding: "10px" }}>
                {functions.map((f, i) => {
                    const parts = f[0].split(/[()]/);
                    const keywordName = parts[0].split(" ");

                    const keywords = keywordName.slice(
                        0,
                        keywordName.length - 1,
                    );
                    const name = keywordName[keywordName.length - 1];
                    const args = parts[1].split(/[,]\s+/).map((a) => a.trim())
                        .filter((a) => a.length > 0);

                    const fbody = f.slice(1, f.length - 1);

                    return (
                        <div
                            key={i}
                            style={{
                                display: "flex",
                                flexDirection: "column",
                                // border: BORDER,
                                marginBottom: "20px",
                                // background: "#272727",
                                background: "#232323",
                                // background: "#202020",
                                // background: "#1f1f1f",
                                color: "#ddd",
                            }}
                        >
                            <FuncHdr
                                name={name}
                                args={args}
                                keywords={keywords.join(" ")}
                            />
                            <div
                                style={{
                                    // height: "100px",
                                    padding: "8px",
                                    // border: BORDER,
                                    borderWidth: "0 2px 2px 2px",
                                }}
                            >
                                <pre>
                                {fbody.map((l, i) => <div className="codeline" key={i}>{l.slice(4)}</div>)}
                                </pre>
                            </div>
                        </div>
                    );
                })}
            </div>
        </div>
    );
}

let functionLines: string[] = [];
const includes: string[] = [];
const locals: string[] = [];
const functions: string[][] = [];
const foo = code.replaceAll("'\n'", "'\\n'")
    .replaceAll("'\0'", "'\\0'");

const strRep = "blahblah";
const strRepRegexp = new RegExp(strRep, "g");

const strRegexp = /"(.|\n)*?"/g;
const strings = [...foo.matchAll(strRegexp)].map((regarr) =>
    regarr[0].replaceAll("\n", "\\n")
);

const foo2 = foo.replaceAll(strRegexp, strRep);

let i = 0;
foo2.split("\n").forEach((origline) => {
    let line = origline;
    const mat = [...origline.matchAll(strRepRegexp)];
    if (mat.length > 0) {
        mat.forEach(() => {
            line = line.replace(strRep, strings[i]);
            i++;
        });
    }

    const functionStarted = functionLines.length > 0;
    const lastChar = line[line.length - 1];

    if (functionStarted) {
        functionLines.push(line);
        if (line === "}") {
            functions.push(functionLines);
            functionLines = [];
        }
    } else {
        if (lastChar === "{") {
            functionLines.push(line);
        } else if (line.length > 0) {
            if (line[0] === "#") {
                includes.push(line);
            } else {
                locals.push(line);
            }
        }
    }
});
