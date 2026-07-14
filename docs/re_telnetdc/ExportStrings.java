import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import java.io.*;

public class ExportStrings extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outPath = getScriptArgs()[0];
        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        DataIterator it = currentProgram.getListing().getDefinedData(true);
        while (it.hasNext()) {
            Data d = it.next();
            if (d.hasStringValue()) {
                pw.printf("%s  %s%n", d.getAddress(), d.getDefaultValueRepresentation());
            }
        }
        pw.close();
        println("strings -> " + outPath);
    }
}
