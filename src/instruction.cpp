#include <sstream>

#include "instruction.h"
#include "decoder.h"
#include "rf.h"

using Format = Instruction::Format;
using Type = Instruction::Type;

struct InstSetItem {
    std::string name;
    uint32_t match;
    uint32_t mask;
    Instruction::Executor function;
};

static const InstSetItem inst_lui = {"lui", 0x37, 0x7f, &Instruction::execute_lui};
static const InstSetItem inst_auipc = {"auipc", 0x17, 0x7f, &Instruction::execute_auipc};
static const InstSetItem inst_addi = {"addi", 0x13, 0x707f, &Instruction::execute_addi};
static const InstSetItem inst_slli = {"slli", 0x1013, 0xfc00707f, &Instruction::execute_slli};
static const InstSetItem inst_slti = {"slti", 0x2013, 0x707f, &Instruction::execute_slti};
static const InstSetItem inst_jal = {"jal", 0x6f, 0x7f, &Instruction::execute_jal};
static const InstSetItem inst_jalr = {"jalr", 0x67, 0x707f, &Instruction::execute_jalr};
static const InstSetItem inst_beq = {"beq", 0x63, 0x707f, &Instruction::execute_beq};
static const InstSetItem inst_bne = {"bne", 0x1063, 0x707f, &Instruction::execute_bne};
static const InstSetItem inst_blt = {"blt", 0x4063, 0x707f, &Instruction::execute_blt};
static const InstSetItem inst_bge = {"bge", 0x5063, 0x707f, &Instruction::execute_bge};
static const InstSetItem inst_bltu = {"bltu", 0x6063, 0x707f, &Instruction::execute_bltu};
static const InstSetItem inst_bgeu = {"bgeu", 0x7063, 0x707f, &Instruction::execute_bgeu};
static const InstSetItem inst_lb = {"lb", 0x3, 0x707f, &Instruction::execute_lb};
static const InstSetItem inst_lh = {"lh", 0x1003, 0x707f, &Instruction::execute_lh};
static const InstSetItem inst_lw = {"lw", 0x2003, 0x707f, &Instruction::execute_lw};
static const InstSetItem inst_lbu = {"lbu", 0x4003, 0x707f, &Instruction::execute_lbu};
static const InstSetItem inst_lhu = {"lhu", 0x5003, 0x707f, &Instruction::execute_lhu};
static const InstSetItem inst_sb = {"sb", 0x23, 0x707f, &Instruction::execute_sb};
static const InstSetItem inst_sh = {"sh", 0x1023, 0x707f, &Instruction::execute_sh};
static const InstSetItem inst_sw = {"sw", 0x2023, 0x707f, &Instruction::execute_sw};
static const InstSetItem inst_sltiu = {"sltiu", 0x3013, 0x707f, &Instruction::execute_sltiu};
static const InstSetItem inst_xori = {"xori", 0x4013, 0x707f, &Instruction::execute_xori};
static const InstSetItem inst_ori = {"ori", 0x6013, 0x707f, &Instruction::execute_ori};
static const InstSetItem inst_andi = {"andi", 0x7013, 0x707f, &Instruction::execute_andi};
static const InstSetItem inst_srai = {"srai", 0x40005013, 0xfc00707f, &Instruction::execute_srai};
static const InstSetItem inst_srli = {"srli", 0x5013, 0xfc00707f, &Instruction::execute_srli};
static const InstSetItem inst_add = {"add", 0x33, 0xfe00707f, &Instruction::execute_add};
static const InstSetItem inst_sub = {"sub", 0x40000033, 0xfe00707f, &Instruction::execute_sub};
static const InstSetItem inst_sll = {"sll", 0x1033, 0xfe00707f, &Instruction::execute_sll};
static const InstSetItem inst_slt = {"slt", 0x2033, 0xfe00707f, &Instruction::execute_slt};
static const InstSetItem inst_sltu = {"sltu", 0x3033, 0xfe00707f, &Instruction::execute_sltu};
static const InstSetItem inst_xor = {"xor", 0x4033, 0xfe00707f, &Instruction::execute_xor};
static const InstSetItem inst_or = {"or", 0x6033, 0xfe00707f, &Instruction::execute_or};
static const InstSetItem inst_and = {"and", 0x7033, 0xfe00707f, &Instruction::execute_and};
static const InstSetItem inst_sra = {"sra", 0x40005033, 0xfe00707f, &Instruction::execute_sra};
static const InstSetItem inst_srl = {"srl", 0x5033, 0xfe00707f, &Instruction::execute_srl};

struct InstSet {
    InstSetItem generated_entry;
    Format format;
    size_t memory_size;
    Type type;

    bool match(uint32_t raw) const { 
        return (raw & generated_entry.mask) == generated_entry.match;
    }
};

static const std::vector<InstSet> instSet = {
   { inst_lui,     Format::U,     0,    Type::ARITHM },
   { inst_auipc,   Format::U,     0,    Type::ARITHM },
   { inst_jal,     Format::J,     0,    Type::JUMP },
   { inst_jalr,    Format::I,     0,    Type::JUMP },
   { inst_beq,     Format::B,     0,    Type::BRANCH },
   { inst_bne,     Format::B,     0,    Type::BRANCH },
   { inst_blt,     Format::B,     0,    Type::BRANCH },
   { inst_bge,     Format::B,     0,    Type::BRANCH },
   { inst_bltu,    Format::B,     0,    Type::BRANCH },
   { inst_bgeu,    Format::B,     0,    Type::BRANCH },
   { inst_lb,      Format::I,     1,    Type::LOAD },
   { inst_lh,      Format::I,     2,    Type::LOAD },
   { inst_lw,      Format::I,     4,    Type::LOAD },
   { inst_lbu,     Format::I,     1,    Type::LOADU },
   { inst_lhu,     Format::I,     2,    Type::LOADU },
   { inst_sb,      Format::S,     1,    Type::STORE },
   { inst_sh,      Format::S,     2,    Type::STORE },
   { inst_sw,      Format::S,     4,    Type::STORE },
   { inst_addi,    Format::I,     0,    Type::ARITHM },
   { inst_slti,    Format::I,     0,    Type::ARITHM },
   { inst_sltiu,   Format::I,     0,    Type::ARITHM },
   { inst_xori,    Format::I,     0,    Type::ARITHM },
   { inst_ori,     Format::I,     0,    Type::ARITHM },
   { inst_andi,    Format::I,     0,    Type::ARITHM },
   { inst_slli,    Format::I,     0,    Type::ARITHM },
   { inst_srai,    Format::I,     0,    Type::ARITHM },
   { inst_srli,    Format::I,     0,    Type::ARITHM },
   { inst_add,     Format::R,     0,    Type::ARITHM },
   { inst_sub,     Format::R,     0,    Type::ARITHM },
   { inst_sll,     Format::R,     0,    Type::ARITHM },
   { inst_slt,     Format::R,     0,    Type::ARITHM },
   { inst_sltu,    Format::R,     0,    Type::ARITHM },
   { inst_xor,     Format::R,     0,    Type::ARITHM },
   { inst_or,      Format::R,     0,    Type::ARITHM },
   { inst_and,     Format::R,     0,    Type::ARITHM },
   { inst_sra,     Format::R,     0,    Type::ARITHM },
   { inst_srl,     Format::R,     0,    Type::ARITHM }
};


const InstSet find_entry(uint32_t raw) {
    for (const auto& x : instSet) {
        if (x.match(raw))
            return x;
    }
    throw std::invalid_argument("No entry found for given instruction");
}


Instruction::Instruction(uint32_t bytes, uint32_t PC) :
    PC(PC),
    new_PC(PC + 4)
{
    InstSet entry = find_entry(bytes);

    name  = entry.generated_entry.name;
    format = entry.format;
    type = entry.type;
    function  = entry.generated_entry.function;
    memory_size = entry.memory_size;

    Decoder decoder;
    decoder.Decode(bytes);
    rs1   = decoder.get_rs1();
    rs2   = decoder.get_rs2();
    rd    = decoder.get_rd();
    imm_v = decoder.get_immediate();
}


const std::string Instruction::get_disasm() const {
    std::ostringstream oss;
    oss << name << " ";
    switch(format) {
        case Format::R:
            oss << rs1 << ", ";
            oss << rs2 << ", ";
            oss << rd;
            break;
        case Format::I:
            oss << rs1 << ", ";
            oss << std::hex << imm_v << std::dec << ", ";
            oss << rd;
            break;
        case Format::S:
        case Format::B:
            oss << rs1 << ", ";
            oss << rs2 << ", ";
            oss << std::hex << imm_v;
            break;
        case Format::U:
        case Format::J:
            oss << std::hex << imm_v << std::dec << ", ";
            oss << rd;
            break;
        default:
            assert(0);
    }
    return oss.str();
}

Instruction::Instruction(const Instruction& other) :
    PC(other.PC),
    new_PC(other.new_PC),
    complete(other.complete),
    name(other.name),
    format(other.format),
    type(other.type),
    rs1(other.rs1),
    rs2(other.rs2),
    rd(other.rd),
    rs1_v(other.rs1_v),
    rs2_v(other.rs2_v),
    rd_v(other.rd_v),
    imm_v(other.imm_v),
    memory_addr(other.memory_addr),
    memory_size(other.memory_size),
    function(other.function)
{}


void Instruction::execute() {
    (this->*function)();
    complete = true;
}

