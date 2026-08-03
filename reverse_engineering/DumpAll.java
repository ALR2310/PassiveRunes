import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolTable;
import java.io.PrintWriter;
import java.io.FileOutputStream;
import java.util.*;

public class DumpAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outPath = args.length > 0 ? args[0] : "D:/tmp/ghidra_dump.txt";
        PrintWriter out = new PrintWriter(new FileOutputStream(outPath));

        out.println("=== IMPORTS ===");
        SymbolTable st = currentProgram.getSymbolTable();
        for (Symbol s : st.getExternalSymbols()) {
            out.println(s.getName());
        }

        out.println();
        out.println("=== FUNCTIONS (non-CRT) ===");
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);

        FunctionIterator fiter = currentProgram.getFunctionManager().getFunctions(true);
        List<Function> funcs = new ArrayList<Function>();
        while (fiter.hasNext()) funcs.add(fiter.next());

        String[] noisePrefixes = new String[] {
            "_scrt", "__scrt", "_GS", "__GS", "_raise", "__raise",
            "_init", "__init", "_CRT", "__security", "__isa",
            "_matherr", "_guard", "__guard", "std::", "operator",
            "_RTC", "__RTC", "_except", "__except", "_CxxThrow",
            "__CxxFrameHandler", "_purecall", "_onexit", "__dllonexit",
            "_configure", "_initterm", "_cinit", "_amsg_exit"
        };

        for (Function f : funcs) {
            if (f.isThunk() || f.isExternal()) continue;
            String name = f.getName();
            boolean noisy = false;
            for (String p : noisePrefixes) {
                if (name.startsWith(p)) { noisy = true; break; }
            }
            if (noisy) continue;

            out.println("---- " + name + " @ " + f.getEntryPoint() + " ----");
            DecompileResults res = decomp.decompileFunction(f, 60, monitor);
            if (res != null && res.decompileCompleted()) {
                out.println(res.getDecompiledFunction().getC());
            } else {
                out.println("(decompile failed)");
            }
            out.println();
        }
        out.flush();
        out.close();
        println("Dump written to " + outPath);
    }
}
