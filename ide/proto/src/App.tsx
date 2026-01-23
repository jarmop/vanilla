import { BytesView } from "./BytesView";
import { BytesViewTable } from "./BytesViewTable";
import { ElfReader } from "./ElfReader";
import { Proto } from "./Proto";

export function App() {
    return (
        <>
            <BytesViewTable />
            {/* <BytesView /> */}
            <ElfReader />
            {/* <Proto /> */}
        </>
    );
}
