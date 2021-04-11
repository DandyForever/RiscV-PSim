#include "funcsim.h"

FuncSim::FuncSim(std::vector<uint8_t>& data, uint32_t PC):
    memory(data),
    rf(),
    PC(PC)
{
    rf.set_stack_pointer(memory.get_stack_pointer());
    rf.validate(Register::Number::s0);
    rf.validate(Register::Number::ra);
}

void FuncSim::step() {
    uint32_t raw_bytes = memory.read_word(PC);
    Instruction instr(raw_bytes, PC);
    rf.read_sources(instr);
    instr.execute();
    memory.load_store(instr);
    rf.writeback(instr);
    //memory.dump();

    std::cout << "0x" << std::hex << PC << ": " << instr.get_disasm() << " " << "(0x" << std::hex << raw_bytes << ")" << std::endl;
    rf.dump();

    PC = instr.get_new_PC();
}

void FuncSim::run(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
        step();
}
