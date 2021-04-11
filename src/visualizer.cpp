#include "visualizer.h"

void Visualizer::print_file() {
    std::ofstream out_file;
    out_file.open("pipeline.dot", std::ios::out);

    out_file << "digraph Pipeline\n{\n\trankdir = TB\n\tnode [shape = \"box\", color = \"black\"]\n\tedge [color = \"black\"]\n\tsubgraph cycle {\n\t\trankdir = TB\n\t\tCYCLE";
    for (int i = 1; i <= fetch.size(); i++)
        out_file << " -> " << i;

    out_file << "\n\t}\n\n";
    out_file << "\tsubgraph fetch {\n\t\trankdir = TB\n\t\tfetch0 [shape = \"record\", label = \"{ FETCH }\", color = \"black\"]\n\t\t";
    for (int i = 1; i <= fetch.size(); i++) {
        out_file << "fetch" << std::dec << i << "[shape = \"record\", label = \"{ ";
       if (fetch[i - 1].is_stall)
           out_file << "STALLED }\", color = \"red\"]\n\t\t";
       else if (fetch[i - 1].is_flush)
           out_file << " FLUSHED | Correct PC: 0x" << std::hex << fetch[i - 1].PC << " }\", color = \"purple\"]\n\t\t";
       else if (fetch[i - 1].is_icache)
           out_file << " PC: 0x" << std::hex << fetch[i - 1].PC << " | ICACHE WAITING }\", color = \"blue\"]\n\t\t";
       else if (fetch[i - 1].is_empty)
           out_file << " EMPTY | PC: " << std::hex << fetch[i - 1].PC << " }\", color = \"black\"]\n\t\t";
       else
           out_file << " (0x" << std::hex << fetch[i - 1].PC << ") " << fetch[i - 1].raw_bytes << " }\", color = \"green\"]\n\t\t";
    }

    out_file << "fetch0";
    for (int i = 1; i <= fetch.size(); i++)
        out_file << " -> fetch" << std::dec << i;
    out_file << "\n\t}\n";

    out_file << "\tsubgraph decode {\n\t\trankdir = TB\n\t\tdecode0 [shape = \"record\", label = \"{ DECODE }\", color = \"black\"]\n\t\t";
    for (int i = 1; i <= decode.size(); i++) {
        out_file << "decode" << std::dec << i << "[shape = \"record\", label = \"{ ";
        if (decode[i - 1].is_stall)
            out_file << "STALLED }\", color = \"red\"]\n\t\t";
        else if (decode[i - 1].is_flush)
            out_file << "FLUSHED }\", color = \"purple\"]\n\t\t";
        else
            out_file << " (0x" << std::hex << decode[i - 1].PC << ") " << decode[i - 1].instr << " }\", color = \"green\"]\n\t\t";
    }
    out_file << "decode0";
    for (int i = 1; i <= decode.size(); i++)
        out_file << " -> decode" << std::dec << i;
    out_file << "\n\t}\n";

    out_file << "\tsubgraph execute {\n\t\trankdir = TB\n\t\texecute0 [shape = \"record\", label = \"{ EXECUTE }\", color = \"black\"]\n\t\t";
    for (int i = 1; i <= execute.size(); i++) {
        out_file << "execute" << std::dec << i << "[shape = \"record\", label = \"{ ";
        if (execute[i - 1].is_stall)
            out_file << "STALLED }\", color = \"red\"]\n\t\t";
        else if (execute[i - 1].is_flush)
            out_file << "FLUSHED }\", color = \"purple\"]\n\t\t";
        else
            out_file << " (0x" << std::hex << execute[i - 1].PC << ") " << execute[i - 1].instr << " }\", color = \"green\"]\n\t\t";
    }
    out_file << "execute0";
    for (int i = 1; i <= execute.size(); i++)
        out_file << " -> execute" << std::dec << i;
    out_file << "\n\t}\n";
    
    out_file << "\tsubgraph memory {\n\t\trankdir = TB\n\t\tmemory0 [shape = \"record\", label = \"{ MEMORY }\", color = \"black\"]\n\t\t";
    for (int i = 1; i <= memory.size(); i++) {
        out_file << "memory" << std::dec << i << "[shape = \"record\", label = \"{ ";
       if (memory[i - 1].is_stall)
           out_file << "STALLED }\", color = \"red\"]\n\t\t";
       else if (fetch[i - 1].is_flush)
           out_file << " (0x" << std::hex << memory[i - 1].PC << ") " << memory[i - 1].instr << " | CAUSE FLUSH }\", color = \"purple\"]\n\t\t";
       else if (fetch[i - 1].is_dcache)
           out_file << " (0x" << std::hex << memory[i - 1].PC << ") " << memory[i - 1].instr << " | DCACHE WAITING }\", color = \"blue\"]\n\t\t";
       else if (fetch[i - 1].is_memop)
           out_file << " (0x" << std::hex << memory[i - 1].PC << ") " << memory[i - 1].instr << " }\", color = \"green\"]\n\t\t";
        else
           out_file << " (0x" << std::hex << memory[i - 1].PC << ") " << memory[i - 1].instr << " | NO MEMORY ACCESS }\", color = \"green\"]\n\t\t";
    }

    out_file << "memory0";
    for (int i = 1; i <= memory.size(); i++)
        out_file << " -> memory" << std::dec << i;
    out_file << "\n\t}\n";

    out_file << "\tsubgraph writeback {\n\t\trankdir = TB\n\t\twriteback0 [shape = \"record\", label = \"{ WRITEBACK }\", color = \"black\"]\n\t\t";
    for (int i = 1; i <= writeback.size(); i++) {
        out_file << "writeback" << std::dec << i << "[shape = \"record\", label = \"{ ";
        if (writeback[i - 1].is_stall)
            out_file << "STALLED }\", color = \"red\"]\n\t\t";
        else
            out_file << " (0x" << std::hex << writeback[i - 1].PC << ") " << writeback[i - 1].instr << " }\", color = \"green\"]\n\t\t";
    }
    out_file << "writeback0";
    for (int i = 1; i <= fetch.size(); i++)
        out_file << " -> writeback" << std::dec << i;
    out_file << "\n\t}\n";

    out_file << "}";
    out_file.close();
}
