#ifndef FORWARDING_UNIT_H
#define FORWARDING_UNIT_H

#include "register.h"
#include "instruction.h"

class ForwardingUnit {
public:

    struct Record {
        uint32_t reg = Register::MAX_NUMBER;
        uint32_t value = NO_VAL32;
    };

private:
    Record bypass_mem;
    Record bypass_exe;

public:
    void set_bypass_mem (Record data) { bypass_mem = data; }
    void set_bypass_exe (Record data) { bypass_exe = data; }

    uint32_t read_sources (Instruction& instr);
    void flush();
};

#endif
