import { Assembler } from "./Assembler";
import { BytesView } from "./BytesView";
import { BytesViewTable } from "./BytesViewTable";
import { ElfReader } from "./ElfReader";
import { Proto } from "./Proto";

export function App() {
    return (
        <>
            <Assembler />
            <hr />
            <BytesViewTable />
            <hr />
            {/* <BytesView /> */}
            <ElfReader />
            <hr />
            <Proto />
        </>
    );
}
