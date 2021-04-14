#include "memory.h"

void PerfsimMemory::process() {

    if (request.cycles_left == 0) {
        if (request.request_type == request_type::read)
            request.data = read(request.addr, request.num_bytes);
        else
            write(request.data, request.addr, request.num_bytes);

        request.is_completed = true;
        request_result.is_ready = true;
        request_result.data = request.data;
    }
}

void PerfsimMemory::send_read_request(uint32_t addr, size_t num_bytes) {
    request.request_type = request_type::read;
    request.is_completed = false;
    request.cycles_left = latency;
    request.num_bytes = num_bytes;
    request.addr = addr;
    request.data = 0xBAAAAAAD;
}

void PerfsimMemory::send_write_request(uint32_t value, uint32_t addr, size_t num_bytes) {
    request.request_type = request_type::write;
    request.is_completed = false;
    request.cycles_left = latency;
    request.num_bytes = num_bytes;
    request.addr = addr;
    request.data = value;
}

void PerfsimMemory::clock() {
    request_result.is_ready = false;
    request_result.data = 0xBAAAAAAD;


    if (request.is_completed) {
        return;
    }

    request.cycles_left -= 1;
    
    process();
}
