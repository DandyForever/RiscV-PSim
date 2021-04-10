#ifndef PERFSIM_H
#define PERFSIM_H

#include "rf.h"
#include "memory.h"
#include "cache.h"
#include "stage_register.h"
#include "elf.h"
#include "elf_manager.h"
#include "consts.h"

class PerfSim {
private:
    ElfManager elfManager;
    PerfMemory memory;
    Cache icache;
    Cache dcache;
    RF rf;
    uint32_t PC;
    uint32_t clocks;
    uint32_t ops;
    uint32_t branch_penalties = 0;
    uint32_t data_stalls = 0;
    uint32_t memory_stalls = 0;
    uint32_t multiple_stalls = 0;
    bool pipeline_not_empty = true;

    bool branch_mispredict = false;
    bool fetch_stall = false;
    bool memory_stall = false;
    bool data_stall = false;
    bool multiple_stall = false;

    struct StageRegisterStore {
        StageRegister FETCH_DECODE;
        StageRegister DECODE_EXE;
        StageRegister EXE_MEM;
        StageRegister MEM_WB;
    } stage_registers;

    // used for feedback from later stages to earlier stages 
    struct WireStore {
        // branch misprediction flush
        uint32_t memory_to_fetch_target = NO_VAL32;
        bool memory_to_all_flush = false;

        bool PC_stage_reg_stall = false;
        bool FD_stage_reg_stall = false;
        bool DE_stage_reg_stall = false;
        bool EM_stage_reg_stall = false;

        // masks of RF registers used at EXE/MEM stages
        uint32_t execute_stage_regs = 0;
        uint32_t memory_stage_regs = 0;
    } wires;

public:
    PerfSim(char* executable_filename);
    void run(uint32_t n);
    
    void step();
    
    void fetch_stage();
    void decode_stage();
    void execute_stage();
    void memory_stage();
    void writeback_stage();
};

#endif
