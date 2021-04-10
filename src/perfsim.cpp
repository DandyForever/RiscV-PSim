#include "perfsim.h"
#include "consts.h"

PerfSim::PerfSim(char* executable_filename)
    : elfManager(executable_filename)
    , mmu(elfManager.getWords())
    , rf()
    , PC(elfManager.getPC())
    , clocks(0)
    , ops(0)
{
    rf.set_stack_pointer(mmu.memory.get_stack_pointer());
    rf.validate(Register::Number::s0);
    rf.validate(Register::Number::ra);
    rf.validate(Register::Number::s1);
    rf.validate(Register::Number::s2);
    rf.validate(Register::Number::s3);
}

void PerfSim::step() {
    mmu.clock();

    writeback_stage();
    memory_stage();
    execute_stage();
    decode_stage();
    fetch_stage();
    
    rf.dump();
    clocks++;

    hu.update_stats();

    hu.print_stats(clocks, ops);
    
    if (!hu.is_stall_FD())
        stage_registers.FETCH_DECODE.clock();

    if (!hu.is_stall_DE())
        stage_registers.DECODE_EXE.clock();

    if (!hu.is_stall_EM())
        stage_registers.EXE_MEM.clock();

        stage_registers.MEM_WB.clock();
    
    hu.reset();
}

void PerfSim::run(uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
        this->step();
}

void PerfSim::fetch_stage() {
    std::cout << "FETCH:     ";
    static bool awaiting_memory_request = false;
    static uint32_t fetch_data = NO_VAL32;

    if (hu.check_stall_FD()) {
        std::cout << "STALLED" << std::endl;
        stage_registers.FETCH_DECODE.write(nullptr);
        return;
    }
    
    PC = hu.handle_mispredict_fetch(PC);

    std::cout << std::hex << "PC: " << PC << std::endl;

    if (mmu.is_icache_busy()) {
        std::cout << "\tWAITING FOR ICACHE" << std::endl;
        stage_registers.FETCH_DECODE.write(nullptr);
        return;
    }

    bool fetch_complete = mmu.fetch(awaiting_memory_request, PC, fetch_data);

    if (fetch_complete) {
        if ((fetch_data == 0 ) | (fetch_data == NO_VAL32)) {
            stage_registers.FETCH_DECODE.write(nullptr);
            std::cout << "Empty" << std::endl;
        } else {
            hu.set_pipe_not_empty();
            Instruction* data = new Instruction(fetch_data, PC);
            std::cout << "\t0x" << std::hex << data->get_PC() << ": "
                      << data->get_disasm() << " "
                      << std::endl;

            stage_registers.FETCH_DECODE.write(data);
            PC = PC + 4;
        }
    } else {
        stage_registers.FETCH_DECODE.write(nullptr);
        hu.set_stall_fetch();
    }
}


void PerfSim::decode_stage() {
    std::cout << "DECODE:    ";

    Instruction* data = nullptr;
    data = stage_registers.FETCH_DECODE.read();

    hu.bypass_stall_FD(data != nullptr);

    if (hu.is_mispredict()) {
        stage_registers.DECODE_EXE.write(nullptr);
        std::cout << "FLUSH" << std::endl;
        if (data != nullptr) delete data;
        return;
    }

    if (data == nullptr) {
        stage_registers.DECODE_EXE.write(nullptr);
        std::cout << "STALLED" << std::endl;
        return;
    }
    hu.set_pipe_not_empty();
    std::cout << "0x" << std::hex << data->get_PC() << ": " << data->get_disasm() << " " << std::endl;

    if (hu.is_data_hazard_decode(static_cast<uint32_t>(data->get_rs1()), static_cast<uint32_t>(data->get_rs2())))
        stage_registers.DECODE_EXE.write(nullptr);
    else {
        this->rf.read_sources(*data);
        stage_registers.DECODE_EXE.write(data);
    }

    std::cout << "\tRead from RF: " << data->get_rs1() << " " \
              << data->get_rs2() << std::endl;
}


void PerfSim::execute_stage() {
    std::cout << "EXECUTE:   ";

    Instruction* data = nullptr;
    data = stage_registers.DECODE_EXE.read();

    hu.bypass_stall_DE(data != nullptr);

    if (hu.is_mispredict()) {
        stage_registers.EXE_MEM.write(nullptr);
        std::cout << "FLUSH" << std::endl;
        if (data != nullptr) delete data;
        return;
    }

    if (data == nullptr) {
        stage_registers.EXE_MEM.write(nullptr);
        std::cout << "STALLED" << std::endl;
        return;
    }
    hu.set_pipe_not_empty();
    
    data->execute();

    hu.set_reg_execute(static_cast<uint32_t>(data->get_rd()));
    stage_registers.EXE_MEM.write(data);

    std::cout << "0x" << std::hex << data->get_PC() << ": " << data->get_disasm() << " "<< std::endl;
}

void PerfSim::memory_stage() {
    std::cout << "MEMORY:    ";
    static uint32_t memory_stage_iterations_complete = 0;
    static bool awaiting_memory_request = false;
    static uint32_t memory_data = NO_VAL32;

    Instruction* data = nullptr;
    data = stage_registers.EXE_MEM.read();

    hu.init_memory_stage();

    if (data == nullptr) {
        stage_registers.MEM_WB.write(nullptr);
        std::cout << "STALLED" << std::endl;
        return;
    }
    hu.set_pipe_not_empty();
    hu.set_reg_memory(static_cast<uint32_t>(data->get_rd()));

    if (data->is_load() | data->is_store()) {
        if (mmu.dcache.is_busy()) {
            std::cout << "WAITING FOR DCACHE" << std::endl;
            hu.set_stall_memory();
            stage_registers.MEM_WB.write(nullptr);
            return;
        }

        if (!awaiting_memory_request) {
            uint32_t addr = data->get_memory_addr() + (memory_stage_iterations_complete * 2);
            size_t num_bytes = (data->get_memory_size() == 1) ? 1 : 2;

            if (data->is_load())
                mmu.process_load(addr, num_bytes);

            if (data->is_store()) {
                memory_data = data->get_rs2_v();
                mmu.process_store(memory_data, addr, num_bytes, memory_stage_iterations_complete == 0);
            }

            awaiting_memory_request = true;
            std::cout << "\tRequest to DCACHE sent" << std::endl;
        }

        auto request = mmu.memory_request_status();

        if (request.is_ready) {
            if (data->is_load()) {
                if (memory_stage_iterations_complete == 0)
                    memory_data = request.data;
                else
                    memory_data |= (request.data << 16);
            }

            awaiting_memory_request = false;
            memory_stage_iterations_complete++;
            std::cout << "Feedback from DCACHE recieved" << std::endl;
        }

        bool memory_operation_complete = \
            (memory_stage_iterations_complete * 2) >= data->get_memory_size();

        if (memory_operation_complete) {
            memory_stage_iterations_complete = 0;
            data->set_rd_v(memory_data);
        } else {
            std::cout << "\tMemory stage iterations complete: " << memory_stage_iterations_complete << std::endl;
            hu.set_stall_memory();
            stage_registers.MEM_WB.write(nullptr);
            return;
        }
    } else {
        std::cout << "NOT a memory operation" << std::endl;
    }

    if (data->is_jump() | data->is_branch())
        if (data->get_new_PC() != data->get_PC() + 4)
            hu.set_mispredict(data->get_new_PC());

    stage_registers.MEM_WB.write(data);

    std::cout << "\t0x" << std::hex << data->get_PC() << ": " << data->get_disasm() << " " << std::endl;

    if (hu.is_mispredict())
        std::cout << "\tbranch misprediction, flush" << std::endl;
} 

void PerfSim::writeback_stage() {
    std::cout << "WRITEBACK: ";
    Instruction* data = nullptr;
    
    data = stage_registers.MEM_WB.read();

    if (data == nullptr) {
        std::cout << "BUBBLE" << std::endl;
        return;
    }
    hu.set_pipe_not_empty();
    std::cout << "0x" << std::hex << data->get_PC() << ": " << data->get_disasm() << " " << std::endl;
    this->rf.writeback(*data);
    ops++;
    delete data;
}
