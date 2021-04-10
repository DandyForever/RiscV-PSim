#ifndef _PORT_H_
#define _PORT_H_

#include "instruction.h"

class StageRegister {
private:
    Instruction* data_in = nullptr;
    Instruction* data_out = nullptr;
public:
    void clock() { data_out = data_in; }
    void write(Instruction* input) { data_in = input; }
    Instruction* read() { return data_out; }
};

#endif
