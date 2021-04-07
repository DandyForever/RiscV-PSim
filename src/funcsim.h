#ifndef FUNCSIM_H
#define FUNCSIM_H

#include "common.h"
#include "rf.h"
#include "memory.h"
#include "elf.h"
#include "elf_manager.h"

class FuncSim {
    private:
        ElfManager elfManager;
        FuncMemory memory;
        RF rf;
        Addr PC = NO_VAL32;
    public:
        FuncSim(char* executable_filename);
        void step();
        void run(uint32 n);
};

#endif

