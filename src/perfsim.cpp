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

    if (icache.is_busy()) {
        std::cout << "\tWAITING FOR ICACHE" << std::endl;
        stage_registers.FETCH_DECODE.write(nullptr);
        return;
    }

    if (!awaiting_memory_request) {
        // send requests to memory
        uint32_t addr = PC;
        icache.send_read_request(addr, 4);
        awaiting_memory_request = true;
        std::cout << "\tRequest to ICACHE sent" << std::endl;
    }

    auto status = icache.get_request_status();
    bool fetch_complete = false;
    if (status.is_ready) {
        fetch_data = status.data;

        std::cout << "\tFeedback from ICACHE recieved" << std::endl;
        
        awaiting_memory_request = false;
        fetch_complete = true;
    }

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
        if (dcache.is_busy()) {
            std::cout << "WAITING FOR DCACHE" << std::endl;
            hu.set_stall_memory();
            stage_registers.MEM_WB.write(nullptr);
            return;
        }

        if (!awaiting_memory_request) {
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
            std::cout << "\tRequest to DCACHE sent" << std::endl;
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
            std::cout << "Feedback from DCACHE recieved" << std::endl;
        }

        bool memory_operation_complete = \
            (memory_stage_iterations_complete * 2) >= data->get_memory_size();

        if (memory_operation_complete) {
            memory_stage_iterations_complete = 0;
            data->set_rd_v(memory_data);
        } else {
            std::cout << "\tmemory_stage_iterations_complete: "
                      << memory_stage_iterations_complete << std::endl;
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
