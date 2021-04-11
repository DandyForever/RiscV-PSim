#include "decoder.h"

void Decoder::Decode(uint32_t byte_instr) {
    uint8_t opcode = 0x7F & byte_instr;
    uint32_t rd_init = (0xF80 & byte_instr) >> 7;

    if ((0x14 & opcode) == 0x14) {
        if (rd_init == 0)
            rd = 0; //32
        else
            rd = rd_init;
        imm = (0xFFFFF000 & byte_instr) >> 12;
        if (imm & 0x80000)
            imm = imm | 0xFFF00000;
    } else if (opcode == 0x6F) {
        if (rd_init == 0)
            rd = 0; //32
        else
            rd = rd_init;
        imm = (0x7FE00000 & byte_instr) >> 21;
        imm = imm | (0x100000 & byte_instr) >> 10;
        imm = imm | (0xFF000 & byte_instr) >> 1;
        imm = imm | (0x80000000 & byte_instr) >> 12;
        if (0x80000000 & byte_instr)
            imm = imm | 0xFFF00000;
    } else if (opcode == 0x33) {
        if (rd_init == 0)
            rd = 0; //32
        else
            rd = rd_init;
        rs1 = (0xF8000 & byte_instr) >> 15;
        rs2 = (0x1F00000 & byte_instr) >> 20;
    } else if (opcode == 0x3 || opcode == 0x13 ||
               opcode == 0x73 || opcode == 0x67) {
        if (rd_init == 0)
            rd = 0; //32
        else
            rd = rd_init;
        rs1 = (0xF8000 & byte_instr) >> 15;
        imm = (0xFFF00000 & byte_instr) >> 20;
        if (imm & 0x800)
            imm = imm | 0xFFFFF000;
    } else if (opcode == 0x23) {
        rs1 = (0xF8000 & byte_instr) >> 15;
        rs2 = (0x1F00000 & byte_instr) >> 20;
        imm = (0xF80 & byte_instr) >> 7;
        imm = imm | (0xFE000000 & byte_instr) >> 20;
        if (imm & 0x800)
            imm = imm | 0xFFFFF000;
    } else if (opcode == 0x63) {
        rs1 = (0xF8000 & byte_instr) >> 15;
        rs2 = (0x1F00000 & byte_instr) >> 20;
        imm = (0xF00 & byte_instr) >> 8;
        imm = imm | (0x7E000000 & byte_instr) >> 21;
        imm = imm | (0x80 & byte_instr) << 3;
        imm = imm | (0x80000000 & byte_instr) >> 20;
        if (imm & 0x800)
            imm = imm | 0xFFFFF000;
    }
}

Decoder::Decoder() {}
