#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

#include "instruction.h"
#include "consts.h"

namespace request_type {
    enum Request {
        read,
        write
    };
}

class Memory {
private:
    std::vector<uint8_t> data;
    
public:
    uint32_t read(uint32_t addr, size_t num_bytes) const {
        uint32_t value = 0;
        for (uint i = 0; i < num_bytes; ++i) {
            uint8_t byte = data[addr + i];
            value |= static_cast<uint32_t>(byte) << (8*i);
        }
        return value;
    }
    void write(uint32_t value, uint32_t addr, size_t num_bytes) {
        for (uint i = 0; i < num_bytes; ++i) {
            uint8_t byte = static_cast<uint8_t>(value >> 8*i); 
            data[addr + i] = byte;
        }
    }

    Memory(std::vector<uint8_t> data) :
        data(std::move(data)) { 
        this->data.resize(400000, 0);
    }
    uint32_t get_stack_pointer() const { return (data.size() - 1) & ~(32 - 1); }

    void dump() {
        for (size_t i = 0; i < data.size(); i++)
            std::cout << data[i];
        std::cout << std::endl;
    }
};


class FuncsimMemory : public Memory {
private:
    void load(Instruction& instr) const {
        uint32_t value = read(instr.get_memory_addr(), instr.get_memory_size());
        instr.set_rd_v(value);
    }

    void store(const Instruction& instr) {
        write(instr.get_rs2_v(), instr.get_memory_addr(), instr.get_memory_size());
    }     

public:
    FuncsimMemory(std::vector<uint8_t> data) : Memory(data) { }

    uint32_t read_word(uint32_t addr) { return read(addr, 4); }
    void load_store(Instruction& instr) {
        if (instr.is_load())
            load(instr);
        else if (instr.is_store())
            store(instr);
    }
};


class PerfsimMemory : public Memory {
public:
    struct RequestResult {
        bool is_ready = false;
        uint32_t data = 0xBAAAAAAD;
    };

private:
    struct Request {
        bool is_completed = true;
        request_type::Request request_type;
        uint32_t addr = 0xBAAAAAAD;
        uint32_t data = 0xBAAAAAAD;
        size_t num_bytes = 0xBAAAAAAD;
        uint32_t cycles_left = 0;
    };

    Request request;
    RequestResult request_result;

    uint32_t latency = 0;

    void process();

public:
    PerfsimMemory(std::vector<uint8_t> data, uint32_t latency):
        Memory(data),
        latency(latency)
    {}

    void clock();
    bool is_busy() { 
        return !request.is_completed; 
    }
    void send_read_request(uint32_t addr, size_t num_bytes);
    void send_write_request(uint32_t value, uint32_t addr, size_t num_bytes);
    RequestResult get_request_status() { return request_result; }
};

#endif
