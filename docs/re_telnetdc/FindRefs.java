import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class FindRefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] targets = getScriptArgs()[0].split(",");
        String outPath = getScriptArgs()[1];
        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        for (String t : targets) {
            Address a = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(t);
            ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(a);
            pw.printf("=== Referenzen auf %s ===%n", t);
            boolean any = false;
            while (refs.hasNext()) {
                Reference r = refs.next();
                Address from = r.getFromAddress();
                Function f = getFunctionContaining(from);
                pw.printf("  %s [%s] (%s)%n", from, f != null ? f.getName() : "?", r.getReferenceType());
                any = true;
            }
            if (!any) pw.printf("  (keine direkten Referenzen gefunden)%n");
        }
        pw.close();
        println("refs -> " + outPath);
    }
}
