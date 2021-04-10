#ifndef FUNCSIM_H
#define FUNCSIM_H

#include <iostream>

#include "rf.h"
#include "memory.h"
#include "elf.h"
#include "elf_manager.h"
#include "consts.h"

class FuncSim {
    private:
        ElfManager elfManager;
        FuncMemory memory;
        RF rf;
        uint32_t PC = NO_VAL32;
    public:
        FuncSim(char* executable_filename);
        void step();
        void run(uint32_t n);
};

#endif

