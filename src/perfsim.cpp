#include "perfsim.h"
#include "consts.h"

PerfSim::PerfSim(char* executable_filename)
    : elfManager(executable_filename)
    , memory(elfManager.getWords(), MEM_LATENCY)
    , icache(memory, CACHE_WAY, CACHE_SET, CACHE_LINE)
    , dcache(memory, CACHE_WAY, CACHE_SET, CACHE_LINE)
    , rf()
    , PC(elfManager.getPC())
    , clocks(0)
    , ops(0)
{
    // setup stack
    rf.set_stack_pointer(memory.get_stack_pointer());
    rf.validate(Register::Number::s0);
    rf.validate(Register::Number::ra);
    rf.validate(Register::Number::s1);
    rf.validate(Register::Number::s2);
    rf.validate(Register::Number::s3);
}

void PerfSim::step() {
    memory.clock();
    icache.clock();
    dcache.clock();

    this->writeback_stage();
    this->memory_stage();
    this->execute_stage();
    this->decode_stage();
    this->fetch_stage();

    std::cout << "STALLS: "
              << hu.FD_stage_reg_stall
              << hu.DE_stage_reg_stall
              << hu.EM_stage_reg_stall
              << std::endl;

    rf.dump();
    clocks++;

    hu.is_any_stall = static_cast<int>(hu.is_branch_mispredict) + \
                    static_cast<int>(hu.is_fetch_stall) + \
                    static_cast<int>(hu.is_memory_stall) + \
                    static_cast<int>(hu.is_data_stall) > 1;
    if (hu.is_any_stall) {
        hu.latency_total++;
        if (hu.is_branch_mispredict) hu.mispredict_penalty += 2;
    } else {
        if (hu.is_fetch_stall || hu.is_memory_stall) {
            hu.latency_memory++;
        }
        if (hu.is_data_stall)
            hu.latency_data_dependency++;
        if (hu.is_branch_mispredict)
            hu.mispredict_penalty+=3;
    }
    if (ops > 0)
        std::cout << "CPI: " << clocks*1.0/ops << std::endl;
    std::cout << std::dec << "Clocks: " << clocks << std::endl;
    std::cout << "Ops: " << ops << std::endl;
    std::cout << "Data stalls: " << hu.latency_data_dependency << std::endl;
    std::cout << "latency_memory: " << hu.latency_memory << std::endl;
    std::cout << "Branch penalties: " << hu.mispredict_penalty << std::endl;
    std::cout << "Multiple stalls: " << hu.latency_total << std::endl;
    std::cout << std::string(50, '-') << std::endl << std::endl;

    if (!hu.FD_stage_reg_stall)
        stage_registers.FETCH_DECODE.clock();

    if (!hu.DE_stage_reg_stall)
        stage_registers.DECODE_EXE.clock();

    if (!hu.EM_stage_reg_stall)
        stage_registers.EXE_MEM.clock();

    if (!false)
        stage_registers.MEM_WB.clock();
    
    hu.FD_stage_reg_stall = \
    hu.DE_stage_reg_stall = \
    hu.EM_stage_reg_stall = false;

    hu.is_branch_mispredict = \
    hu.is_data_stall = \
    hu.is_fetch_stall = \
    hu.is_memory_stall = \
    hu.is_any_stall = false;

    if (!hu.is_pipe_not_empty) return;
    hu.is_pipe_not_empty = false;
}

void PerfSim::run(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
        this->step();
}

void PerfSim::fetch_stage() {
    std::cout << "FETCH:  ";
    static bool awaiting_memory_request = false;
    static uint32_t fetch_data = NO_VAL32;

    if (hu.FD_stage_reg_stall) {
        std::cout << "BUBBLE" << std::endl;
        stage_registers.FETCH_DECODE.write(nullptr);
        return;
    }
    
    // branch mispredctiion handling
    if (hu.memory_to_all_flush) {
        fetch_data = NO_VAL32;
        awaiting_memory_request = false;
        PC = hu.memory_to_fetch_target;
        std::cout << "FLUSH, ";
    }

    std::cout << std::hex << "PC: " << PC << std::endl;

    if (icache.is_busy()) {
        std::cout << "\tWAITING ICACHE" << std::endl;
        stage_registers.FETCH_DECODE.write(nullptr);
        return;
    }

    if (!awaiting_memory_request) {
        // send requests to memory
        uint32_t addr = PC;
        icache.send_read_request(addr, 4);
        awaiting_memory_request = true;
        std::cout << "\tsent request to icache" << std::endl;
    }

    auto request = icache.get_request_status();
    bool fetch_complete = false;
    if (request.is_ready) {
        fetch_data = request.data;

        std::cout << "\tgot request from icache" << std::endl;
        
        awaiting_memory_request = false;
        fetch_complete = true;
    }

    if (fetch_complete) {
        if ((fetch_data == 0 )| (fetch_data == NO_VAL32)) {
            stage_registers.FETCH_DECODE.write(nullptr);
            std::cout << "Empty" << std::endl;
        } else {
            hu.is_pipe_not_empty = true;
            Instruction* data = new Instruction(fetch_data, PC);
            std::cout << "\t0x" << std::hex << data->get_PC() << ": "
                      << data->get_disasm() << " "
                      << std::endl;

            stage_registers.FETCH_DECODE.write(data);
            PC = PC + 4;
        }
    } else {
        stage_registers.FETCH_DECODE.write(nullptr);
        hu.is_fetch_stall = true;
        return;
    }
}


void PerfSim::decode_stage() {
    std::cout << "DECODE: ";

    Instruction* data = nullptr;
    data = stage_registers.FETCH_DECODE.read();


    if (hu.DE_stage_reg_stall & (data != nullptr))
        hu.FD_stage_reg_stall = true;

    // branch mispredctiion handling
    if (hu.memory_to_all_flush) {
        stage_registers.DECODE_EXE.write(nullptr);
        std::cout << "FLUSH" << std::endl;
        if (data != nullptr) delete data;
        return;
    }
    

    if (data == nullptr) {
        stage_registers.DECODE_EXE.write(nullptr);
        std::cout << "BUBBLE" << std::endl;
        return;
    }
    hu.is_pipe_not_empty = true;
    std::cout << "0x" << std::hex << data->get_PC() << ": "
              << data->get_disasm() << " "
              << std::endl;

    // read RF registers mask
    uint32_t decode_stage_regs = \
        (1 << static_cast<uint32_t>(data->get_rs1()))
      | (1 << static_cast<uint32_t>(data->get_rs2()));

    uint32_t hazards = \
        (decode_stage_regs & hu.execute_stage_regs)
      | (decode_stage_regs & hu.memory_stage_regs);

    if ((hazards >> 1) != 0) {
        hu.is_data_stall = true;
        hu.FD_stage_reg_stall = true;
        stage_registers.DECODE_EXE.write(nullptr);
    } else {
        this->rf.read_sources(*data);
        stage_registers.DECODE_EXE.write(data);
    }

    std::cout << "\tRegisters read: " << data->get_rs1() << " " \
              << data->get_rs2() << std::endl;
}


void PerfSim::execute_stage() {
    std::cout << "EXE:    ";

    Instruction* data = nullptr;
    data = stage_registers.DECODE_EXE.read();

    hu.execute_stage_regs = 0;

    if (hu.EM_stage_reg_stall & (data != nullptr))
        hu.DE_stage_reg_stall = true;

    // branch mispredctiion handling
    if (hu.memory_to_all_flush) {
        stage_registers.EXE_MEM.write(nullptr);
        std::cout << "FLUSH" << std::endl;
        if (data != nullptr) delete data;
        return;
    }

    if (data == nullptr) {
        stage_registers.EXE_MEM.write(nullptr);
        std::cout << "BUBBLE" << std::endl;
        return;
    }
    hu.is_pipe_not_empty = true;
    // actual execution takes place here
    data->execute();
    hu.execute_stage_regs = (1 << static_cast<uint32_t>(data->get_rd()));
    stage_registers.EXE_MEM.write(data);

    std::cout << "0x" << std::hex << data->get_PC() << ": "
                      << data->get_disasm() << " "
                      << std::endl;
}

void PerfSim::memory_stage() {
    std::cout << "MEM:    ";
    static uint32_t memory_stage_iterations_complete = 0;
    static bool awaiting_memory_request = false;
    static uint32_t memory_data = NO_VAL32;

    Instruction* data = nullptr;
    data = stage_registers.EXE_MEM.read();

    hu.memory_to_all_flush = false;
    hu.memory_to_fetch_target = NO_VAL32;
    hu.memory_stage_regs = 0;

    if (data == nullptr) {
        stage_registers.MEM_WB.write(nullptr);
        std::cout << "BUBBLE" << std::endl;
        return;
    }
    hu.is_pipe_not_empty = true;
    hu.memory_stage_regs = (1 << static_cast<uint32_t>(data->get_rd()));

    // memory operations
    if (data->is_load() | data->is_store()) {
        if (dcache.is_busy()) {
            std::cout << "WAITING DCACHE" << std::endl;
            hu.EM_stage_reg_stall = true;
            stage_registers.MEM_WB.write(nullptr);
            hu.is_memory_stall = true;
            return;
        }

        if (!awaiting_memory_request) {
            // send requests to memory
            uint32_t addr = data->get_memory_addr() + (memory_stage_iterations_complete * 2);
            size_t num_bytes = (data->get_memory_size() == 1) ? 1 : 2;

            if (data->is_load()) {
                std::cout << "READING at " << std::hex << addr << std::endl;
                dcache.send_read_request(addr, num_bytes);
            }

            if (data->is_store()) {
                memory_data = data->get_rs2_v();
                std::cout << "WRITING " << std::hex << memory_data << " at " << std::hex << addr << std::endl;
                if (memory_stage_iterations_complete == 0)
                    dcache.send_write_request(memory_data, addr, num_bytes);
                else
                    dcache.send_write_request(memory_data >> 16, addr, num_bytes);
            }

            awaiting_memory_request = true;
            std::cout << "\tsent request to dcache" << std::endl;
        }

        auto request = dcache.get_request_status();

        if (request.is_ready) {
            if (data->is_load()) {
                if (memory_stage_iterations_complete == 0)
                    memory_data = request.data;
                else
                    memory_data |= (request.data << 16);
            }

            awaiting_memory_request = false;
            memory_stage_iterations_complete++;
            std::cout << "GOT request from dcache" << std::endl;
        }

        bool memory_operation_complete = \
            (memory_stage_iterations_complete * 2) >= data->get_memory_size();

        if (memory_operation_complete) {
            memory_stage_iterations_complete = 0;
            data->set_rd_v(memory_data);
        } else {
            std::cout << "\tmemory_stage_iterations_complete: "
                      << memory_stage_iterations_complete << std::endl;
            hu.EM_stage_reg_stall = true;
            stage_registers.MEM_WB.write(nullptr);
            hu.is_memory_stall = true;
            return;
        }
    } else {
        std::cout << "NOT a memory operation" << std::endl;
    }

    // jump operations
    if (data->is_jump() | data->is_branch()) {
        if (data->get_new_PC() != data->get_PC() + 4) {
            // target misprediction handling
            hu.memory_to_all_flush = true;
            hu.memory_to_fetch_target = data->get_new_PC();
            hu.is_branch_mispredict = true;
        }
    }

    // pass data to writeback stage
    stage_registers.MEM_WB.write(data);

    std::cout << "\t0x" << std::hex << data->get_PC() << ": "
              << data->get_disasm() << " "
              << std::endl;

    if (hu.memory_to_all_flush)
        std::cout << "\tbranch misprediction, flush" << std::endl;
} 


void PerfSim::writeback_stage() {
    std::cout << "WB:     ";
    Instruction* data = nullptr;
    
    data = stage_registers.MEM_WB.read();

    if (data == nullptr) {
        std::cout << "BUBBLE" << std::endl;
        return;
    }
    hu.is_pipe_not_empty = true;
    std::cout << "0x" << std::hex << data->get_PC() << ": "
          << data->get_disasm() << " "
          << std::endl;
    this->rf.writeback(*data);
    ops++;
    delete data;
}
