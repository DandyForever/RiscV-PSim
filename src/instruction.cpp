#include <sstream>

#include "instruction.h"
#include "decoder.h"
#include "rf.h"

using Format = Instruction::Format;
using Type = Instruction::Type;

struct ISAEntryGenerated {
    std::string name;
    uint32_t match;
    uint32_t mask;
    Instruction::Executor function;
};

#define DECLARE_INSN(name, match, mask) \
static const ISAEntryGenerated ISA_entry_generated_ ## name = \
{ #name, match, mask, &Instruction::execute_ ## name};
#include "opcodes.gen.h"
#undef DECLARE_INSN

struct ISAEntry {
    ISAEntryGenerated generated_entry;
    Format format;
    size_t memory_size;
    Type type;

    bool match(uint32_t raw) const { 
        return (raw & generated_entry.mask) == generated_entry.match;
    }
};


#define I(name) \
ISA_entry_generated_ ## name

#define F(format) \
Instruction::Format::format

#define T(type) \
Instruction::Type::type

static const std::vector<ISAEntry> ISA_table = {
   { I(lui),     F(U),     0,    T(ARITHM) },
   { I(auipc),   F(U),     0,    T(ARITHM) },
   { I(jal),     F(J),     0,    T(JUMP) },
   { I(jalr),    F(I),     0,    T(JUMP) },
   { I(beq),     F(B),     0,    T(BRANCH) },
   { I(bne),     F(B),     0,    T(BRANCH) },
   { I(blt),     F(B),     0,    T(BRANCH) },
   { I(bge),     F(B),     0,    T(BRANCH) },
   { I(bltu),    F(B),     0,    T(BRANCH) },
   { I(bgeu),    F(B),     0,    T(BRANCH) },
   { I(lb),      F(I),     1,    T(LOAD) },
   { I(lh),      F(I),     2,    T(LOAD) },
   { I(lw),      F(I),     4,    T(LOAD) },
   { I(lbu),     F(I),     1,    T(LOADU) },
   { I(lhu),     F(I),     2,    T(LOADU) },
   { I(lwu),     F(I),     4,    T(LOADU) },
   { I(sb),      F(S),     1,    T(STORE) },
   { I(sh),      F(S),     2,    T(STORE) },
   { I(sw),      F(S),     4,    T(STORE) },
   { I(addi),    F(I),     0,    T(ARITHM) },
   { I(slti),    F(I),     0,    T(ARITHM) },
   { I(sltiu),   F(I),     0,    T(ARITHM) },
   { I(xori),    F(I),     0,    T(ARITHM) },
   { I(ori),     F(I),     0,    T(ARITHM) },
   { I(andi),    F(I),     0,    T(ARITHM) },
   { I(slli),    F(I),     0,    T(ARITHM) },
   { I(srai),    F(I),     0,    T(ARITHM) },
   { I(srli),    F(I),     0,    T(ARITHM) },
   { I(add),     F(R),     0,    T(ARITHM) },
   { I(sub),     F(R),     0,    T(ARITHM) },
   { I(sll),     F(R),     0,    T(ARITHM) },
   { I(slt),     F(R),     0,    T(ARITHM) },
   { I(sltu),    F(R),     0,    T(ARITHM) },
   { I(xor),     F(R),     0,    T(ARITHM) },
   { I(or),      F(R),     0,    T(ARITHM) },
   { I(and),     F(R),     0,    T(ARITHM) },
   { I(sra),     F(R),     0,    T(ARITHM) },
   { I(srl),     F(R),     0,    T(ARITHM) }
};


const ISAEntry find_entry(uint32_t raw) {
    for (const auto& x : ISA_table) {
        if (x.match(raw))
            return x;
    }
    throw std::invalid_argument("No entry found for given instruction");
}


Instruction::Instruction(uint32_t bytes, uint32_t PC) :
    PC(PC),
    new_PC(PC + 4)
{
    ISAEntry entry = find_entry(bytes);

    name  = entry.generated_entry.name;
    format = entry.format;
    type = entry.type;
    function  = entry.generated_entry.function;
    memory_size = entry.memory_size;

    //Decoder decoder(bytes, format);
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
            oss << rd << ", ";
            oss << std::hex << imm_v;
            break;
        case Format::S:
        case Format::B:
            oss << rs1 << ", ";
            oss << rs2 << ", ";
            oss << std::hex << imm_v;
            break;
        case Format::U:
        case Format::J:
            oss << rd << ", ";
            oss << std::hex << imm_v;
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

