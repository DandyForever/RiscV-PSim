#ifndef CACHE_H
#define CACHE_H

#include "memory.h"
#include "consts.h"

#include <queue>
#include <numeric>
#include <iostream>



class Cache {
public:
    struct RequestResult {
        bool is_ready = false;
        uint32_t data = 0xBAAAAAAD;
    };
    Cache(PerfsimMemory& memory, uint32_t num_ways, uint32_t num_sets, uint32_t line_size_in_bytes);
    void clock();
    bool is_busy() { return !request.is_completed; }
    void send_read_request(uint32_t addr, uint32_t num_bytes);
    void send_write_request(uint32_t value, uint32_t addr, uint32_t num_bytes);
    RequestResult get_request_status();
private:
    struct Line {
        std::vector<uint8_t> data;
        uint32_t addr = 0xBAAAAAAD;
        bool is_valid = false;
        bool is_dirty = false;


        Line(uint32_t size_in_bytes) :
            data(size_in_bytes)
        { }

        uint32_t read_bytes(uint32_t offset, uint32_t num_bytes);
        void write_bytes(uint32_t value, uint32_t offset, uint32_t num_bytes);
    };

    PerfsimMemory& memory;
    
    uint32_t num_sets;
    uint32_t line_size_in_bytes;


    std::vector<std::queue<uint32_t>> fifo_queues;
    std::vector<std::vector<Line>> cache_mem;

    struct Request {
        bool is_completed = true;
        request_type::Request request_type = request_type::read;
        uint32_t addr = 0xBAAAAAAD;
        uint32_t data = 0xBAAAAAAD;
        uint32_t num_bytes = 0xBAAAAAAD;
    };

    struct LineRequest {
        request_type::Request request_type = request_type::read;
        bool awaiting_memory_request = false;
        uint32_t addr = 0xBAAAAAAD;
        uint32_t set = 0xBAAAAAAD;
        uint32_t way = 0xBAAAAAAD;
        uint32_t bytes_processed = 0;

        LineRequest(uint32_t addr, uint32_t set, uint32_t way, request_type::Request request_type)
            : request_type(request_type), addr(addr), set(set), way(way)
        { }
    };

    Request request;

    std::queue<LineRequest> line_requests;

    void process();
    bool process_called_this_cycle = false;

    void process_line_requests();

    uint get_set(uint32_t addr) const { return (addr / line_size_in_bytes) & (num_sets - 1); }
    uint32_t get_tag(uint32_t addr) const { return (addr / line_size_in_bytes); }
    uint32_t get_line_addr(uint32_t addr) const { return addr - get_line_offset(addr); }
    uint32_t get_line_offset(uint32_t addr) const { return addr % this->line_size_in_bytes; }

    std::pair<bool, uint32_t> lookup(uint32_t addr);

public:

};

#endif
