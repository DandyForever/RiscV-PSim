#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
#include "register.h"

enum class Opcode : uint8_t {
    LUI = 0x37,
    AUIPC = 0x17,
    JAL = 0x6F,
    JALR = 0x67,
    BRANCH = 0x63,
    LOAD = 0x3,
    STORE = 0x23,
    OP_IMM = 0x13,
    OP = 0x33,
    MISC_MEM = 0xF,
    SYSTEM = 0x73,
};
class Decoder {
private:

    uint8_t rs1;
    uint8_t rs2;
    uint8_t rd;

    uint32_t imm;
public:
    Decoder();
    Register get_rs1() { 
        if (rs1 >= 0 && rs1 <= 31)
            return Register(rs1); 
        return Register::zero();
    }
    Register get_rs2() { 
        if (rs2 >= 0 && rs2 <= 31)
            return Register(rs2); 
        return Register::zero();
    }
    Register get_rd() { 
        if (rd >= 0 && rd <= 31)
            return Register(rd); 
        return Register::zero();
    }

    uint32_t get_immediate() { return imm; }

    ~Decoder(){}

    void Decode(uint32_t instr);
};
