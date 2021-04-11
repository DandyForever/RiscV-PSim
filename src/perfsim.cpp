#include "perfsim.h"
#include "consts.h"

PerfSim::PerfSim(std::vector<uint8_t>& data, uint32_t PC): 
    mmu(data),
    rf(),
    PC(PC),
    clocks(0),
    ops(0)
{
    rf.set_stack_pointer(mmu.getSP());
    rf.validate(Register::Names::s0);
    rf.validate(Register::Names::ra);
    rf.validate(Register::Names::s1);
    rf.validate(Register::Names::s2);
    rf.validate(Register::Names::s3);
}

void PerfSim::step() {
    mmu.clock();

    writeback_stage();
    memory_stage();
    execute_stage();
    decode_stage();
    fetch_stage();
    
    rf.dump();
    mmu.dump();
    clocks++;

    hu.update_stats();
    fu.flush();
    
    if (!hu.is_stall_FD())
        latch.FETCH_DECODE.clock();

    if (!hu.is_stall_DE())
        latch.DECODE_EXE.clock();

    if (!hu.is_stall_EM())
        latch.EXE_MEM.clock();

        latch.MEM_WB.clock();
    
    hu.reset();
}

void PerfSim::run(uint32_t n) {
    while (ops < n)
        step();

    visual.print_file();
    hu.print_stats(clocks, ops);
}

void PerfSim::fetch_stage() {
    Visualizer::Record record;
    static bool awaiting_memory_request = false;
    static uint32_t fetch_data = NO_VAL32;

    if (hu.check_stall_FD()) {
        record.is_stall = true;
        visual.record_fetch(record);
        latch.FETCH_DECODE.write(nullptr);
        return;
    }
    
    if (hu.is_mispredict()) {
        awaiting_memory_request = false;
        record.is_flush = true;
        PC = hu.get_real_PC();
    }

    record.PC = PC;

    if (mmu.is_icache_busy()) {
        record.is_icache = true;
        latch.FETCH_DECODE.write(nullptr);
        visual.record_fetch(record);
        return;
    }

    bool fetch_complete = mmu.fetch(awaiting_memory_request, PC, fetch_data);

    record.raw_bytes = fetch_data;

    if (fetch_complete) {
        if ((fetch_data == 0 ) | (fetch_data == NO_VAL32)) {
            latch.FETCH_DECODE.write(nullptr);
            record.is_empty = true;
        } else {
            hu.set_pipe_not_empty();
            Instruction* data = new Instruction(fetch_data, PC);
            record.instr = data->get_disasm();

            latch.FETCH_DECODE.write(data);
            PC = PC + 4;
        }
    } else {
        latch.FETCH_DECODE.write(nullptr);
        record.is_icache = true;
        hu.set_stall_fetch();
    }
    
    visual.record_fetch(record);
}


void PerfSim::decode_stage() {
    Visualizer::Record record;

    Instruction* data = nullptr;
    data = latch.FETCH_DECODE.read();

    hu.bypass_stall_FD(data != nullptr);

    if (hu.is_mispredict()) {
        latch.DECODE_EXE.write(nullptr);
        record.is_flush = true;
        if (data != nullptr) delete data;
        visual.record_decode(record);
        return;
    }

    if (data == nullptr) {
        latch.DECODE_EXE.write(nullptr);
        record.is_stall = true;
        visual.record_decode(record);
        return;
    }
    hu.set_pipe_not_empty();

    record.PC = data->get_PC();
    record.instr = data->get_disasm();

    if (hu.is_data_hazard_decode(static_cast<uint32_t>(data->get_rs1()), static_cast<uint32_t>(data->get_rs2())))
        latch.DECODE_EXE.write(nullptr);
    else {
        rf.read_sources(*data);
        auto bypass_info = fu.read_sources(*data);
        if (bypass_info == 2)
            record.is_bypass_exe = true;
        else if (bypass_info == 1)
            record.is_bypass_mem = true;
        latch.DECODE_EXE.write(data);
    }

    visual.record_decode(record);
}


void PerfSim::execute_stage() {
    Visualizer::Record record;

    Instruction* data = nullptr;
    data = latch.DECODE_EXE.read();

    hu.bypass_stall_DE(data != nullptr);

    if (hu.is_mispredict()) {
        latch.EXE_MEM.write(nullptr);
        record.is_flush = true;
        visual.record_execute(record);
        if (data != nullptr) delete data;
        return;
    }

    if (data == nullptr) {
        latch.EXE_MEM.write(nullptr);
        record.is_stall = true;
        visual.record_execute(record);
        return;
    }
    hu.set_pipe_not_empty();
    
    data->execute();

    //hu.set_reg_execute(static_cast<uint32_t>(data->get_rd())); //Not necessary
    fu.set_bypass_exe({static_cast<uint32_t>(data->get_rd()), data->get_rd_v()});
    latch.EXE_MEM.write(data);

    record.PC = data->get_PC();
    record.instr = data->get_disasm();

    visual.record_execute(record);
}

void PerfSim::memory_stage() {
    Visualizer::Record record;
    static uint32_t memory_stage_iterations_complete = 0;
    static bool awaiting_memory_request = false;
    static uint32_t memory_data = NO_VAL32;

    Instruction* data = nullptr;
    data = latch.EXE_MEM.read();

    hu.init_memory_stage();

    if (data == nullptr) {
        latch.MEM_WB.write(nullptr);
        record.is_stall = true;
        visual.record_memory(record);
        return;
    }
    hu.set_pipe_not_empty();
    if (data->is_load())
        hu.set_reg_memory(static_cast<uint32_t>(data->get_rd()));
    else
        hu.set_reg_memory(Register::MAX_NUMBER);

    record.PC = data->get_PC();
    record.instr = data->get_disasm();

    if (data->is_load() | data->is_store()) {
        record.is_memop = true;
        if (mmu.is_dcache_busy()) {
            hu.set_stall_memory();
            latch.MEM_WB.write(nullptr);
            record.is_dcache = true;
            visual.record_memory(record);
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
        }

        bool memory_operation_complete = (memory_stage_iterations_complete * 2) >= data->get_memory_size();

        if (memory_operation_complete) {
            memory_stage_iterations_complete = 0;
            data->set_rd_v(memory_data);
            fu.set_bypass_mem({static_cast<uint32_t>(data->get_rd()), memory_data});
            hu.set_reg_memory(static_cast<uint32_t>(Register::MAX_NUMBER));
            record.is_memop = true;
        } else {
            hu.set_stall_memory();
            latch.MEM_WB.write(nullptr);
            record.is_dcache = true;
            visual.record_memory(record);
            return;
        }
    } else {//Not a memory operation
        fu.set_bypass_mem({static_cast<uint32_t>(data->get_rd()), data->get_rd_v()});
    }

    if (data->is_jump() | data->is_branch())
        if (data->get_new_PC() != data->get_PC() + 4)
            hu.set_mispredict(data->get_new_PC());

    latch.MEM_WB.write(data);

    if (hu.is_mispredict()) {
        record.is_flush = true;
    }

    visual.record_memory(record);
} 

void PerfSim::writeback_stage() {
    Visualizer::Record record;
    Instruction* data = nullptr;
    
    data = latch.MEM_WB.read();

    if (data == nullptr) {
        record.is_stall = true;
        visual.record_writeback(record);
        return;
    }
    record.PC = data->get_PC();
    record.instr = data->get_disasm();
    visual.record_writeback(record);
    hu.set_pipe_not_empty();
    std::cout << "0x" << std::hex << data->get_PC() << ": " << data->get_disasm() << " " << std::endl;
    rf.writeback(*data);
    ops++;
    delete data;
}
