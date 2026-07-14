import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import java.io.*;

public class FixEntry extends GhidraScript {
    @Override
    public void run() throws Exception {
        long entryOff = Long.decode(getScriptArgs()[0]);
        Address base = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(0);
        Address entry = base.add(entryOff);

        DisassembleCommand cmd = new DisassembleCommand(entry, null, true);
        cmd.applyTo(currentProgram, monitor);

        createFunction(entry, "real_entry");

        // AutoAnalyse laufen lassen, damit Funktionsgrenzen/Referenzen konsistent werden
        analyzeAll(currentProgram);

        String outPath = getScriptArgs().length > 1 ? getScriptArgs()[1] : "/tmp/listing2.txt";
        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext()) {
            Instruction instr = it.next();
            Function f = getFunctionContaining(instr.getAddress());
            String fname = (f != null) ? f.getName() : "?";
            pw.printf("%s [%s]  %s%n", instr.getAddress(), fname, instr.toString());
        }
        pw.close();
        println("fertig: " + outPath);
    }
}
