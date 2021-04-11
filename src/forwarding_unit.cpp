#include "forwarding_unit.h"

uint32_t ForwardingUnit::read_sources(Instruction& instr) {
    uint32_t rs1 = static_cast<uint32_t>(instr.get_rs1());    
    uint32_t rs2 = static_cast<uint32_t>(instr.get_rs2());
    bool is_mem1 = false;
    bool is_mem2 = false;
    bool is_exe1 = false;
    bool is_exe2 = false;

    if (bypass_mem.reg == rs1 && bypass_mem.reg != Register::MAX_NUMBER && bypass_mem.reg != 0) {
        instr.set_rs1_v(bypass_mem.value);
        is_mem1 = true;
    }
    if (bypass_mem.reg == rs2 && bypass_mem.reg != Register::MAX_NUMBER && bypass_mem.reg != 0) {
        instr.set_rs2_v(bypass_mem.value);
        is_mem2 = true;
    }

    if (bypass_exe.reg == rs1 && bypass_exe.reg != Register::MAX_NUMBER && bypass_exe.reg != 0) {
        instr.set_rs1_v(bypass_exe.value);
        is_exe1 = true;
    }
    if (bypass_exe.reg == rs2 && bypass_exe.reg != Register::MAX_NUMBER && bypass_exe.reg != 0) {
        instr.set_rs2_v(bypass_exe.value);
        is_exe2 = true;
    }

    if ((is_mem1 && is_exe2) || (is_mem2 && is_exe1))
        return 3;
    if (is_exe1 || is_exe2)
        return 2;
    if (is_mem1 || is_mem2)
        return 1;
    return 0;
}

void ForwardingUnit::flush() {
    set_bypass_mem({Register::MAX_NUMBER, NO_VAL32});
    set_bypass_exe({Register::MAX_NUMBER, NO_VAL32});
}
