#include "forwarding_unit.h"

uint32_t ForwardingUnit::read_sources(Instruction& instr) {
    uint32_t rs1 = static_cast<uint32_t>(instr.get_rs1());    
    uint32_t rs2 = static_cast<uint32_t>(instr.get_rs2());
    bool is_mem = false;
    bool is_exe = false;

    if (bypass_mem.reg == rs1) {
        instr.set_rs1_v(bypass_mem.value);
        is_mem = true;
    }
    if (bypass_mem.reg == rs2) {
        instr.set_rs2_v(bypass_mem.value);
        is_mem = true;
    }

    if (bypass_exe.reg == rs1) {
        instr.set_rs1_v(bypass_exe.value);
        is_exe = true;
    }
    if (bypass_exe.reg == rs2) {
        instr.set_rs2_v(bypass_exe.value);
        is_exe = true;
    }

    if (is_exe)
        return 2;
    if (is_mem)
        return 1;
    return 0;
}

void ForwardingUnit::flush() {
    set_bypass_mem({Register::MAX_NUMBER, NO_VAL32});
    set_bypass_exe({Register::MAX_NUMBER, NO_VAL32});
}
