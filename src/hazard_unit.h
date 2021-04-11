#ifndef PSIM_HAZARD_UNIT_H
#define PSIM_HAZARD_UNIT_H


class HazardUnit {
public:
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

    uint32_t execute_stage_regs = 0;
    uint32_t memory_stage_regs = 0;
};


#endif //PSIM_HAZARD_UNIT_H
