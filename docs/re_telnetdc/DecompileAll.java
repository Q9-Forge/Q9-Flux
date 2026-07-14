import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.*;
import java.io.*;

public class DecompileAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outPath = getScriptArgs()[0];
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        FunctionIterator it = currentProgram.getListing().getFunctions(true);
        while (it.hasNext()) {
            Function f = it.next();
            DecompileResults res = decomp.decompileFunction(f, 60, monitor);
            pw.printf("// ===== %s @ %s =====%n", f.getName(), f.getEntryPoint());
            if (res != null && res.decompileCompleted()) {
                pw.println(res.getDecompiledFunction().getC());
            } else {
                pw.println("// decompile failed");
            }
            pw.println();
        }
        pw.close();
        println("done -> " + outPath);
    }
}
