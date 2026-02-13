export type Program = {
    name: string;
    args: string[];
    keywords: string[];
    body: [
        {
            type: string;
            name: string;
            args: string[];
        },
    ];
};
