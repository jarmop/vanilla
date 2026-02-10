import React from "react";
import { code } from "../data/codestrings/ide.js";

const BORDER = "1px solid #666";

export function ProgramEditor() {
    return (
        <div
            className="ProgramEditor"
            style={{ width: "100%", height: "100%" }}
        >
            <div
                style={{ width: "fit-content", padding: "10px" }}
            >
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

                    console.log(args);

                    return (
                        <div
                            key={i}
                            style={{
                                display: "flex",
                                flexDirection: "column",
                                // border: BORDER,
                                marginBottom: "20px",
                                // background: "#202020",
                                background: "#272727",
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
                                    height: "100px",
                                    padding: "8px",
                                    // border: BORDER,
                                    borderWidth: "0 2px 2px 2px",
                                }}
                            >
                                Function body
                            </div>
                        </div>
                    );
                })}
            </div>
        </div>
    );
}

interface FuncHdrProps {
    name: string;
    args: string[];
    keywords: string;
}

function FuncHdr({ name, args, keywords }: FuncHdrProps) {
    return (
        <div
            style={{
                display: "flex",
                // border: BORDER,
                background: "#202020",
                // background: "#272727",
            }}
        >
            <div
                style={{
                    // border: BORDER,
                    padding: "8px",
                }}
            >
                {name}
            </div>
            {
                args.length > 0 &&
                (
                    <div
                        style={{
                            // border: BORDER,
                            padding: "8px",
                            whiteSpace: "nowrap",
                            width: "100%",
                        }}
                    >
                        {args.map((arg) => {
                            const parts = arg.split(" ");
                            const name = parts[parts.length - 1];
                            const types = parts.slice(0, -1);

                            // return `${types} ${name}`;
                            return (
                                <React.Fragment>
                                    <span
                                        style={{
                                            color: "#9DF",
                                            // background: "#3a3a3a",
                                            background: "#373737",
                                            padding: "2px 4px",
                                            borderRadius: "4px",
                                            // borderRadius: "10px / 4px",
                                            // borderRadius: "5% / 50%",
                                            marginRight: "5px",
                                        }}
                                    >
                                        <span
                                            style={{
                                                color: "#5B5",
                                                // display: "none",
                                            }}
                                        >
                                            {types.join(" ")}
                                            &nbsp;
                                        </span>
                                        <span
                                            style={{
                                                color: "#9DF",
                                                // background: "#3a3a3a",
                                                // padding: "2px 4px",
                                                // borderRadius: "4px",
                                            }}
                                        >
                                            {name}
                                        </span>
                                    </span>
                                    {/* {i < args.length - 1 ? "," : ""} */}
                                    {/* &nbsp; */}
                                </React.Fragment>
                            );
                        })}
                    </div>
                )
                // Maybe args row appears automatically when nonlocal variable
                // is used. If the variable gets declared locally or globally,
                // the row column disappears again.
                // : (
                //     <div
                //         style={{
                //             display: "flex",
                //             border: BORDER,
                //             padding: "8px",
                //             // fontSize: "16px",
                //             whiteSpace: "nowrap",
                //         }}
                //     >
                //         {/* + */}
                //     </div>
                // )
            }
            <div
                style={{
                    display: "none",
                    border: BORDER,
                    padding: "8px",
                    width: "100%",
                    whiteSpace: "nowrap",
                }}
            >
                {keywords}
            </div>
        </div>
    );
}

let functionLines: string[] = [];
const includes: string[] = [];
const locals: string[] = [];
const functions: string[][] = [];
code.split("\n").forEach((line) => {
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

    // const lastChar = line[line.length - 1];
    // if (lastChar === "{" && !functionStarted) {
    //     functionLines.push(line);
    // } else if (line === "}" && functionStarted) {
    //     functions.push(functionLines);
    //     functionLines = [];
    // }
});

// console.log(functions.map((f) => f[0]));
// console.log(locals);
