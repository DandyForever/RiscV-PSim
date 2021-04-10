#ifndef CACHE_H
#define CACHE_H

#include "memory.h"
#include "consts.h"

#include <list>
#include <queue>
#include <numeric>
#include <iostream>
#include <cassert>

using Set = uint32_t;
using Way = uint32_t;

class LRUInfo {
private:
    std::vector<  // set
        std::list<  // list of ways ordered by usage time
            Way  // way
        >
    > lru;

public:
    LRUInfo(uint32_t ways, uint32_t sets);

    // make given way most recently used within set
    void touch(Set set, Way way);

    // get least recently used way in a set
    Way get_LRU_way(Set set);
};

class Cache {
public:
    struct RequestResult {
        bool is_ready = false;
        uint32_t data = NO_VAL32;
    };

private:
    // single cache line
    struct Line {
        std::vector<uint8_t> data;
        uint32_t addr = NO_VAL32;
        bool is_valid = false;
        bool is_dirty = false;


        Line(uint32_t size_in_bytes) :
            data(size_in_bytes)
        { }

        uint32_t read_bytes(uint32_t offset, uint32_t num_bytes);
        void write_bytes(uint32_t value, uint32_t offset, uint32_t num_bytes);
    };

    // underlying memory (or next-level cache)
    PerfMemory& memory;
    
    // cache params
    uint32_t num_sets;
    uint32_t line_size_in_bytes;

    // main array of lines
    std::vector<  // way
        std::vector<  // set
            Line
        >
    > array;

    // special object to evict LRU lines
    LRUInfo lru_info;

    // read/write data request to caсhe
    struct Request {
        bool complete = true;
        bool is_read = false;
        uint32_t addr = NO_VAL32;
        uint32_t data = NO_VAL32;
        uint32_t num_bytes = NO_VAL32;
    };

    // line read/write request to memory
    struct LineRequest {
        bool is_read = false;
        bool awaiting_memory_request = false;
        uint32_t addr = NO_VAL32;
        Set set = NO_VAL32;
        Way way = NO_VAL32;
        uint32_t bytes_processed = 0;

        LineRequest(uint32_t addr, Set set, Way way, bool is_read)
            : is_read(is_read)
            , addr(addr)
            , set(set)
            , way(way)
        { }
    };

    // active request to cache (single-port cache)
    Request request;

    // queue of read/write line requests to memory
    // to be processed
    std::queue<LineRequest> line_requests;

    // process active request to cache
    void process();
    bool process_called_this_cycle = false;
    void process_miss();
    void process_hit(Way way);
    void process_line_requests();

    // helper functions
    uint get_set(uint32_t addr) const { return (addr / line_size_in_bytes) & (num_sets - 1); }
    uint32_t get_tag(uint32_t addr) const { return (addr / line_size_in_bytes); }
    /*uint32_t get_line_addr(uint32_t addr) const { return addr & ~(this->line_size_in_bytes - 1); }
    uint32_t get_line_offset(uint32_t addr) const { return addr & (this->line_size_in_bytes - 1); }*/
    uint32_t get_line_addr(uint32_t addr) const { return addr - get_line_offset(addr); }
    uint32_t get_line_offset(uint32_t addr) const { return addr % this->line_size_in_bytes; }
    // check whether particular address is present in cache
    std::pair<bool, Way> lookup(uint32_t addr);

public:
    Cache(PerfMemory& memory,
          uint32_t num_ways,
          uint32_t num_sets,
          uint32_t line_size_in_bytes);
    void clock();
    bool is_busy() { return !request.complete; }
    void send_read_request(uint32_t addr, uint32_t num_bytes);
    void send_write_request(uint32_t value, uint32_t addr, uint32_t num_bytes);
    RequestResult get_request_status();
};

#endif
