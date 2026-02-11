import { FuncHdr } from "./FuncHdr.tsx";

export function ProgramCustom() {
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
                        name="Start"
                        args={["int a1", "int a2"]}
                        keywords={["staic", "void"].join(" ")}
                    />
                    <div
                        style={{
                            // height: "100px",
                            padding: "8px",
                            // border: BORDER,
                            borderWidth: "0 2px 2px 2px",
                        }}
                    >
                        <div>
                            Function body
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}
