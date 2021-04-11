#include "mmu.h"

MMU::MMU(const std::vector<uint8_t>& data):
    memory(data, MEM_LATENCY),
    icache(memory, CACHE_WAY, CACHE_SET, CACHE_LINE),
    dcache(memory, CACHE_WAY, CACHE_SET, CACHE_LINE) {}

void MMU::clock() {
   memory.clock();
   icache.clock();
   dcache.clock();
}

void MMU::dump() { 
    if (!IS_DUMP_MEM)
        return;

    memory.dump(); 
}

bool MMU::fetch(bool& is_request, uint32_t PC, uint32_t& data) {
   if (!is_request) {
       icache.send_read_request(PC, 4);
       is_request = true;
   }

   auto status = icache.get_request_status();
   if (status.is_ready) {
       data = status.data;

       is_request = false;
       return true;
   }
   return false;
}

void MMU::process_load(uint32_t addr, size_t num_bytes) {
    dcache.send_read_request(addr, num_bytes);
}

void MMU::process_store(uint32_t data, uint32_t addr, size_t num_bytes, bool is_complete) {
    if (is_complete)
        dcache.send_write_request(data, addr, num_bytes);
    else
        dcache.send_write_request(data >> 16, addr, num_bytes);
}
