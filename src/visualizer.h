#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <vector>
#include <fstream>


class Visualizer {
public:
    struct Record {
        bool is_stall = false;
        bool is_icache = false;
        bool is_dcache = false;
        bool is_memop = false;
        bool is_flush = false;
        bool is_empty = false;
        bool is_bypass_exe = false;
        bool is_bypass_mem = false;
        std::string instr = "";
        uint32_t raw_bytes = 0;
        uint32_t PC = 0;
    };

private:
    std::vector <Record> fetch;
    std::vector <Record> decode;
    std::vector <Record> execute;
    std::vector <Record> memory;
    std::vector <Record> writeback;

public:
    Visualizer(){}
    
    void record_fetch(Record record) { fetch.push_back(record); }
    void record_decode(Record record) { decode.push_back(record); }
    void record_execute(Record record) { execute.push_back(record); }
    void record_memory(Record record) { memory.push_back(record); }
    void record_writeback(Record record) { writeback.push_back(record); }

    void print_file();
};

#endif
