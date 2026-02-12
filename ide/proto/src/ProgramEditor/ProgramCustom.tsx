import { useState } from "react";
import { FuncHdr } from "./FuncHdr.tsx";
import type { Program } from "./types.ts";

const defaultProgram: Program = {
    name: "start",
    args: [],
    keywords: ["int"],
    body: [
        {
            type: "call",
            name: "print",
            args: ["hello"],
        },
    ],
};

export function ProgramCustom() {
    const [program, setProgram] = useState(defaultProgram);

    function compile() {
        fetch("http://localhost:8000", {
            method: "POST",
            // body: JSON.stringify({ message: "Hello from regreg!" }),
            body: JSON.stringify(program),
            // …
        }).then((response) => response.json()).then((json) =>
            console.log(json)
        );
    }

    function setArgs(line: number, args: string[]) {
        const newProgram = { ...program };
        newProgram.body[line].args = args;
        setProgram(newProgram);
    }

    return (
        <div
            className="ProgramEditor"
            style={{ width: "100%", height: "100%" }}
        >
            <div style={{ width: "fit-content", padding: "10px" }}>
                <div
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
                        name={program.name}
                        args={program.args}
                        keywords={program.keywords.join(" ")}
                    />
                    <div
                        style={{
                            // height: "100px",
                            padding: "8px",
                            // border: BORDER,
                            borderWidth: "0 2px 2px 2px",
                        }}
                    >
                        {program.body.map((line, i) => (
                            <div key={i} style={{ display: "flex" }}>
                                <div style={{ marginRight: "10px" }}>
                                    {line.name}
                                </div>
                                <div>
                                    <input
                                        type="text"
                                        value={line.args[0]}
                                        onChange={(e) =>
                                            setArgs(i, [e.target.value])}
                                    />
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            </div>
            <button type="button" onClick={compile}>Compile</button>
        </div>
    );
}
