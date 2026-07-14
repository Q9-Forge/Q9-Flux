import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;

public class ExportListing extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outPath = getScriptArgs().length > 0 ? getScriptArgs()[0] : "/tmp/listing.txt";
        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext()) {
            Instruction instr = it.next();
            String comment = "";
            Function f = getFunctionContaining(instr.getAddress());
            String fname = (f != null) ? f.getName() : "?";
            pw.printf("%s [%s]  %-40s  %s%n", instr.getAddress(), fname, instr.toString(), instr.getComment(CodeUnit.EOL_COMMENT) == null ? "" : instr.getComment(CodeUnit.EOL_COMMENT));
        }
        pw.close();
        println("Listing exportiert nach " + outPath);
    }
}
