#ifndef FUNCSIM_H
#define FUNCSIM_H

#include <iostream>

#include "rf.h"
#include "memory.h"
#include "elf.h"
#include "consts.h"

class FuncSim {
    private:
        FuncsimMemory memory;
        RF rf;
        uint32_t PC = NO_VAL32;
    public:
        FuncSim(std::vector<uint8_t>& data, uint32_t PC);
        void step();
        void run(uint32_t n);
};

#endif

