#ifndef FUNCSIM_H
#define FUNCSIM_H

#include "common.h"
#include "rf.h"
#include "memory.h"
#include "elf.h"

class FuncSim {
    private:
        ElfLoader loader;
        FuncMemory memory;
        RF rf;
        Addr PC = NO_VAL32;
    public:
        FuncSim(std::string executable_filename);
        void step();
        void run(uint32 n);
};

#endif

