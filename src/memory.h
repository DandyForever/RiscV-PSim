#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

#include "instruction.h"
#include "consts.h"

class Memory {
private:
    std::vector<uint8_t> data;

    uint8_t read_byte(uint32_t addr) const {
        return data[addr];
    }

    void write_byte(uint8_t value, uint32_t addr) {
        data[addr] = value;
    }
    
public:
    uint32_t read(uint32_t addr, size_t num_bytes) const;
    void write(uint32_t value, uint32_t addr, size_t num_bytes);

    Memory(std::vector<uint8_t> data);
    uint32_t get_stack_pointer() const { return (data.size() - 1) & ~(32 - 1); }

    void dump() {
        for (size_t i = 0; i < data.size(); i++)
            std::cout << data[i];
        std::cout << std::endl;
    }
};


class FuncMemory : public Memory {
private:
    void load(Instruction& instr) const {
        uint32_t value = read(instr.get_memory_addr(), instr.get_memory_size());
        instr.set_rd_v(value);
    }

    void store(const Instruction& instr) {
        write(instr.get_rs2_v(), instr.get_memory_addr(), instr.get_memory_size());
    }     

public:
    FuncMemory(std::vector<uint8_t> data) : Memory(data) { }

    uint32_t read_word(uint32_t addr) { return read(addr, 4); }
    void load_store(Instruction& instr) {
        if (instr.is_load())
            load(instr);
        else if (instr.is_store())
            store(instr);
    }
};


class PerfMemory : public Memory {
public:
    struct RequestResult {
        bool is_ready = false;
        uint32_t data = NO_VAL32;
    };

private:
    struct Request {
        bool complete = true;
        bool is_read = false;
        uint32_t addr = NO_VAL32;
        uint32_t data = NO_VAL32;
        size_t num_bytes = NO_VAL32;
        uint32_t cycles_left_to_complete = 0;
    };

    Request request;
    RequestResult request_result;

    uint32_t latency_in_cycles = 0;

    void process();

public:
    PerfMemory(std::vector<uint8_t> data, uint32_t latency_in_cycles):
        Memory(data),
        latency_in_cycles(latency_in_cycles)
    {}

    void clock();
    bool is_busy() { return !request.complete; }
    void send_read_request(uint32_t addr, size_t num_bytes);
    void send_write_request(uint32_t value, uint32_t addr, size_t num_bytes);
    RequestResult get_request_status() { return request_result; }
};

#endif
