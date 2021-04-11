#include "hazard_unit.h"

void HazardUnit::update_stats() {
    is_any_stall = (static_cast<int>(is_branch_mispredict) + static_cast<int>(is_fetch_stall) + static_cast<int>(is_memory_stall) + static_cast<int>(is_data_stall)) > 1;
    if (is_any_stall) {
        latency_total++;
        if (is_branch_mispredict) 
            mispredict_penalty += 2;
    } else {
        if (is_fetch_stall || is_memory_stall) {
            latency_memory++;
        }
        if (is_data_stall)
            latency_data_dependency++;
        if (is_branch_mispredict)
            mispredict_penalty += 3;
    }
}

void HazardUnit::print_stats(const uint32_t cycles, const uint32_t instructions) const {
    std::cout << "\nStats summary:" << std::endl;
    if (instructions > 0)
        std::cout << "CPI: " << cycles * 1.0 / instructions << std::endl;
    std::cout << std::dec << "Cycles: " << cycles << std::endl;
    std::cout << "Instructions: " << instructions << std::endl;
    std::cout << "Data dependency stalls: " << latency_data_dependency << std::endl;
    std::cout << "Memory latency: " << latency_memory << std::endl;
    std::cout << "Misprediction penalty: " << mispredict_penalty << std::endl;
}

void HazardUnit::reset() {
    FD_stage_reg_stall = false;
    DE_stage_reg_stall = false;
    EM_stage_reg_stall = false;

    is_branch_mispredict = false;
    is_data_stall = false;
    is_fetch_stall = false;
    is_memory_stall = false;
    is_any_stall = false;

    is_pipe_not_empty = false;
}

uint32_t HazardUnit::handle_mispredict_fetch(uint32_t PC, bool& is_request) {
    if (memory_to_all_flush) {
        is_request = false;
        std::cout << "FLUSH, ";
        return memory_to_fetch_target;
    } else
        return PC;
}

void HazardUnit::bypass_stall_FD(bool is_data) {
    if (DE_stage_reg_stall && is_data)
        FD_stage_reg_stall = true;
}

void HazardUnit::bypass_stall_DE(bool is_data) {
    execute_stage_regs = 0;

    if (EM_stage_reg_stall && is_data)
        DE_stage_reg_stall = true;
}

bool HazardUnit::is_data_hazard_decode(uint32_t rs1, uint32_t rs2) {
    uint32_t decode_regs = (1 << rs1) | (1 << rs2);
    uint32_t hazard = (decode_regs & execute_stage_regs) | (decode_regs & memory_stage_regs);
    hazard = hazard >> 1;
    if (hazard) {
        is_data_stall = true;
        FD_stage_reg_stall = true;
    }
    return hazard != 0;
}

void HazardUnit::init_memory_stage() {
    memory_to_all_flush = false;
    memory_to_fetch_target = NO_VAL32;
    memory_stage_regs = 0;
}

void HazardUnit::set_stall_memory() {
    EM_stage_reg_stall = true;
    is_memory_stall = true;
}

void HazardUnit::set_mispredict(uint32_t PC) {
    memory_to_all_flush = true;
    memory_to_fetch_target = PC;
    is_branch_mispredict = true;
}
