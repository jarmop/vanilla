interface FuncHdrProps {
    name: string;
    args: string[];
    keywords: string;
}

export function FuncHdr({ name, args, keywords }: FuncHdrProps) {
    return (
        <div
            style={{
                display: "flex",
                // border: BORDER,
                // background: "#202020",
                background: "#1a1a1a",
                // background: "#272727",
            }}
        >
            <div
                style={{
                    padding: "8px",
                }}
            >
                {name}
            </div>
            {args.length > 0 &&
                (
                    <div
                        style={{
                            padding: "8px",
                            whiteSpace: "nowrap",
                            width: "100%",
                        }}
                    >
                        {args.map((arg, i) => {
                            const parts = arg.split(" ");
                            const name = parts[parts.length - 1];
                            const types = parts.slice(0, -1);
                            return (
                                <span
                                    key={i}
                                    style={{
                                        color: "#9DF",
                                        // background: "#3a3a3a",
                                        // background: "#373737",
                                        // background: "#343434",
                                        // background: "#323232",
                                        // background: "#303030",
                                        // background: "#272727",
                                        background: "#2a2a2a",
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
                            );
                        })}
                    </div>
                )}
            <div
                style={{
                    display: "none",
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
