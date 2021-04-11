#ifndef PERFSIM_H
#define PERFSIM_H

#include "rf.h"
#include "mmu.h"
#include "stage_register.h"
#include "elf.h"
#include "consts.h"
#include "hazard_unit.h"

class PerfSim {
private:
    MMU mmu;
    RF rf;
    HazardUnit hu;
    uint32_t PC;
    
    uint32_t clocks;
    uint32_t ops;

    struct StageRegisterStore {
        StageRegister FETCH_DECODE;
        StageRegister DECODE_EXE;
        StageRegister EXE_MEM;
        StageRegister MEM_WB;
    } stage_registers;

public:
    PerfSim(std::vector<uint8_t>& data, uint32_t PC);
    void run(uint32_t n);
    
    void step();
    
    void fetch_stage();
    void decode_stage();
    void execute_stage();
    void memory_stage();
    void writeback_stage();
};

#endif
