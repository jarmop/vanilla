export function leftPad(str: string, n: number) {
    let pStr = str;
    while (pStr.length < n) {
        pStr = "0" + pStr;
    }
    return pStr;
}
