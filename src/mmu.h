#ifndef PSIM_MMU_H
#define PSIM_MMU_H

#include <vector>

#include "memory.h"
#include "cache.h"
#include "consts.h"

class MMU {
public:
    PerfMemory memory;
    Cache icache;
    Cache dcache;

public:
    MMU(const std::vector<uint8_t>& data);

    void clock();
    bool is_icache_busy() { return icache.is_busy(); }
    bool is_dcache_busy() { return dcache.is_busy(); }
    bool fetch(bool& is_request, uint32_t PC, uint32_t& data);
    void process_load(uint32_t addr, size_t num_bytes);
    void process_store(uint32_t data, uint32_t addr, size_t num_bytes, bool is_complete);
    Cache::RequestResult memory_request_status() { return dcache.get_request_status(); } 
};

#endif //PSIM_MMU_H
