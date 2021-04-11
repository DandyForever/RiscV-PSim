#ifndef PSIM_HAZARD_UNIT_H
#define PSIM_HAZARD_UNIT_H

#include <iostream>
#include "consts.h"

class HazardUnit {
private:
    uint32_t mispredict_penalty = 0;
    uint32_t latency_data_dependency = 0;
    uint32_t latency_memory = 0;
    uint32_t latency_total = 0;

    bool is_pipe_not_empty = true;

    bool is_branch_mispredict = false;
    bool is_fetch_stall = false;
    bool is_data_stall = false;
    bool is_memory_stall = false;
    bool is_any_stall = false;

    uint32_t memory_to_fetch_target = NO_VAL32;
    bool memory_to_all_flush = false;

    bool PC_stage_reg_stall = false;
    bool FD_stage_reg_stall = false;
    bool DE_stage_reg_stall = false;
    bool EM_stage_reg_stall = false;

    uint32_t execute_stage_regs = 32;
    uint32_t memory_stage_regs = 32;

public:
    void update_stats();
    void print_stats(const uint32_t cycles, const uint32_t instructions) const;
    void reset();

    bool check_stall_FD() { return FD_stage_reg_stall; }
    void set_pipe_not_empty() { is_pipe_not_empty = true; }
    void set_stall_fetch() { is_fetch_stall = true; }

    bool is_stall_FD() { return FD_stage_reg_stall; }
    bool is_stall_DE() { return DE_stage_reg_stall; }
    bool is_stall_EM() { return EM_stage_reg_stall; }

    void bypass_stall_FD(bool is_data);
    void bypass_stall_DE(bool is_data);
    void init_memory_stage();

    uint32_t handle_mispredict_fetch(uint32_t PC, bool& is_request);
    bool is_mispredict() { return memory_to_all_flush; }
    void set_mispredict(uint32_t PC);
    
    uint32_t get_real_PC() { return memory_to_fetch_target; }

    bool is_data_hazard_decode(uint32_t rs1, uint32_t rs2);
    void set_stall_memory();

    void set_reg_execute(uint32_t rd) { execute_stage_regs = 1 << rd; }
    void set_reg_memory(uint32_t rd) { memory_stage_regs = 1 << rd; }
};


#endif //PSIM_HAZARD_UNIT_H
