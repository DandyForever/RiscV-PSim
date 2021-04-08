#ifndef PSIM_FUNCTIONAL_SIMULATOR_H
#define PSIM_FUNCTIONAL_SIMULATOR_H

#include "common.h"
#include "rf.h"
#include "memory.h"
#include "elf_manager.h"

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


#endif //PSIM_FUNCTIONAL_SIMULATOR_H
