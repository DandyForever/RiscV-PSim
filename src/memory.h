#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

#include "instruction.h"
#include "consts.h"

class Memory {
private:
    std::vector<uint8_t> data;

    uint8_t read_byte(uint32_t addr) const {
        return this->data[addr];
    }

    void write_byte(uint8_t value, uint32_t addr) {
        this->data[addr] = value;
    }
    
public:
    uint32_t read(uint32_t addr, size_t num_bytes) const;
    void write(uint32_t value, uint32_t addr, size_t num_bytes);

public:
    Memory(std::vector<uint8_t> data);
    uint32_t get_stack_pointer() const { return (data.size() - 1) & ~(32 - 1); }
};


class FuncMemory : public Memory {
private:
    void load(Instruction& instr) const {
        uint32_t value = this->read(instr.get_memory_addr(), instr.get_memory_size());
        instr.set_rd_v(value);
    }

    void store(const Instruction& instr) {
        this->write(instr.get_rs2_v(), instr.get_memory_addr(), instr.get_memory_size());
    }     

public:
    FuncMemory(std::vector<uint8_t> data) : Memory(data) { }

    uint32_t read_word(uint32_t addr) { return this->read(addr, 4); }
    void load_store(Instruction& instr) {
        if (instr.is_load())
            this->load(instr);
        else if (instr.is_store())
            this->store(instr);
    }
};


class PerfMemory : public Memory {
public:
    struct RequestResult {
        bool is_ready = false;
        uint32_t data = NO_VAL32;
    };

private:
    // read/write request to memory
    struct Request {
        bool complete = true;
        bool is_read = false;
        uint32_t addr = NO_VAL32;
        uint32_t data = NO_VAL32;
        size_t num_bytes = NO_VAL32;
        uint32_t cycles_left_to_complete = 0;
    };

    // active request to memory (single-port memory)
    Request request;
    RequestResult request_result;

    // fixed memory latency
    uint32_t latency_in_cycles = 0;

    // process active request to memory
    void process();

public:
    PerfMemory(std::vector<uint8_t> data,
               uint32_t latency_in_cycles)
    : Memory(data)
    , latency_in_cycles(latency_in_cycles)
    { }

    void clock();
    bool is_busy() { return !request.complete; }
    void send_read_request(uint32_t addr, size_t num_bytes);
    void send_write_request(uint32_t value, uint32_t addr, size_t num_bytes);
    RequestResult get_request_status() { return request_result; }
};

#endif
