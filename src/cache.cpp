#include "cache.h"
#include <sstream>

uint32_t Cache::Line::read_bytes(uint32_t offset, uint32_t num_bytes) {
    uint32_t value = 0;
    for (uint i = 0; i < num_bytes; ++i) {
        uint8_t byte = data[offset + i];
        value |= static_cast<uint32_t>(byte) << (8*i);
    }
    return value;
}

void Cache::Line::write_bytes(uint32_t value, uint32_t offset, uint32_t num_bytes) {
    for (uint i = 0; i < num_bytes; ++i) {
        uint8_t byte = static_cast<uint8_t>(value >> 8*i); 
        data[offset + i] = byte;
    }
}

Cache::Cache(PerfsimMemory& memory, uint32_t num_ways, uint32_t num_sets, uint32_t line_size_in_bytes)
    : memory(memory)
    , num_sets(num_sets)
    , line_size_in_bytes(line_size_in_bytes)
    , cache_mem(num_ways, std::vector<Line>(num_sets, Line(line_size_in_bytes)))
    , fifo_queues(num_sets, std::queue<uint32_t>())
    {
        for (auto i = 0; i < num_sets; i++) 
            for (auto j = 0; j < num_ways; j++) 
                fifo_queues[i].push(j);
    }


void Cache::process_line_requests() {
    if (line_requests.empty())
        return;

    if (memory.is_busy())
        return;

    auto& line_request = line_requests.front();
    Line& line = cache_mem[line_request.way][line_request.set];

    if (line_request.awaiting_memory_request) {
        auto mr = memory.get_request_status();
        
        if (line_request.request_type == request_type::read)
            line.write_bytes(mr.data, line_request.bytes_processed, 2);

        line_request.awaiting_memory_request = false;
        line_request.bytes_processed += 2;
    }

    if (line_request.bytes_processed == line.data.size()) {
        if (line_request.request_type == request_type::read) {
            line.is_valid = true;
            line.addr = line_request.addr;
            line.is_dirty = false;
        }
        else {
            line.is_valid = true;
            line.is_dirty = false;
        }

        line_requests.pop();

        process_line_requests(); 
    }
    else if (!line_request.awaiting_memory_request) {
        // send requests to memory
        if (line_request.request_type == request_type::read)
            memory.send_read_request(line_request.addr + line_request.bytes_processed, 2);
        else
            memory.send_write_request(line.read_bytes(line_request.bytes_processed, 2),
                                            line_request.addr + line_request.bytes_processed, 2);
        line_request.awaiting_memory_request = true;
    }
}

void Cache::process() {
    auto& r = request;  // alias

    if (line_requests.empty()) {
        const auto [is_hit, way] = lookup(r.addr);
        if (is_hit) {
            auto& r = request;  // alias

            uint32_t set = get_set(r.addr);
            Line& line = cache_mem[way][set];

            uint32_t offset = get_line_offset(r.addr);
            if (r.request_type == request_type::read) {
                r.data = line.read_bytes(offset, r.num_bytes);
            }
            else {
                line.write_bytes(r.data, offset, r.num_bytes);
                line.is_dirty = true;
            }
            r.is_completed = true;
        }
        else {
            uint32_t set = get_set(request.addr);
            uint32_t way = fifo_queues[set].front();
            fifo_queues[set].pop();
            fifo_queues[set].push(way);

            Line& line = cache_mem[way][set];

            if (line.is_valid && line.is_dirty) {
                line_requests.push(
                    LineRequest(get_line_addr(line.addr), set, way, request_type::write)
                );
            }

            line_requests.push(
                LineRequest(get_line_addr(request.addr), set, way, request_type::read)
            );
        }
    }
    process_line_requests();
}


std::pair<bool, uint32_t> Cache::lookup(uint32_t addr) {
    const auto set = get_set(addr);
    const auto tag = get_tag(addr);
    for (uint way = 0; way < cache_mem.size(); ++way) {
        auto& line = cache_mem[way][set];
        if (get_tag(line.addr) == tag && line.is_valid) {
            return {true, way};
        }
    }
    return {false, 0xBAAAAAAD};
}

void Cache::send_read_request(uint32_t addr, uint32_t num_bytes) {
    request.request_type = request_type::read;
    request.is_completed = false;
    request.num_bytes = num_bytes;
    request.addr = addr;
    request.data = 0xBAAAAAAD;

    process();
    process_called_this_cycle = true;
}

void Cache::send_write_request(uint32_t value, uint32_t addr, uint32_t num_bytes) {
    request.request_type = request_type::write;
    request.is_completed = false;
    request.num_bytes = num_bytes;
    request.addr = addr;
    request.data = value;

    process();
    process_called_this_cycle = true;
}

void Cache::clock() {
    if (request.is_completed)
        return;

    if (!process_called_this_cycle)
        process();

    process_called_this_cycle = false;
}

Cache::RequestResult Cache::get_request_status() {
    if (request.is_completed)
        return RequestResult {true, request.data};
    else
        return RequestResult {false, 0xBAAAAAAD};
}
