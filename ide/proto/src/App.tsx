import { Assembler } from "./Assembler";
import { BytesView } from "./BytesView";
import { BytesViewTable } from "./BytesViewTable";
import { ElfReader } from "./ElfReader";
import { ProgramEditor } from "./ProgramEditor/ProgramEditor.tsx";
import { Proto } from "./Proto";

export function App() {
    return (
        <>
            <ProgramEditor />
            {/* <hr /> */}
            {/* <Assembler /> */}
            {/* <hr /> */}
            {/* <BytesViewTable /> */}
            {/* <hr /> */}
            {/* <BytesView /> */}
            {/* <ElfReader /> */}
            {/* <hr /> */}
            {/* <Proto /> */}
        </>
    );
}
