#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cassert>
#include <vector>
#include <iostream>

#include "register.h"
#include "consts.h"

class Instruction {
public:
    // RISCV encoding format
    enum class Format {
        R, I, S, B, U, J,
        UNKNOWN
    };

    // internal types
    enum class Type {
        LOADU, LOAD, STORE,
        ARITHM,
        JUMP, BRANCH,
        UNKNOWN
    };

    // executor function type
    using Executor = void (Instruction::*)(void);

private:
    // PC
    const uint32_t PC = NO_VAL32;
    uint32_t new_PC = NO_VAL32;

    // main info
    bool complete = false;
    std::string name = "unknown";
    Format format = Format::UNKNOWN;
    Type type = Type::UNKNOWN;

    // registers
    Register rs1 = Register::zero();
    Register rs2 = Register::zero();
    Register rd  = Register::zero();

    // registers values
    uint32_t rs1_v = NO_VAL32;
    uint32_t rs2_v = NO_VAL32;
    uint32_t rd_v  = NO_VAL32;
    
    // immediate
    int32_t imm_v = NO_VAL32;

    // for loads/stores
    uint32_t memory_addr = NO_VAL32;
    uint32_t memory_size = NO_VAL32;

public:
    // constructors
    explicit Instruction(uint32_t bytes, uint32_t PC);
    Instruction(const Instruction& other);
    Instruction() = delete;

    // dummy getters
    const Register get_rs1 () const { return rs1; }
    const Register get_rs2 () const { return rs2; }
    const Register get_rd  () const { return rd; }

    bool is_sign_extended_load () const { return type == Type::LOAD; }
    bool is_zero_extended_load () const { return type == Type::LOADU; }
    bool is_load  () const { return is_sign_extended_load() || is_zero_extended_load(); }
    bool is_store () const { return type == Type::STORE; }
    bool is_jump () const { return (type == Type::JUMP); }
    bool is_branch () const { return (type == Type::BRANCH); }
    
    void set_rs1_v (uint32_t value) { rs1_v = value; }
    void set_rs2_v (uint32_t value) { rs2_v = value; }
    void set_rd_v  (uint32_t value) {  rd_v = value; }

    uint32_t get_rs1_v () const { return rs1_v; }
    uint32_t get_rs2_v () const { return rs2_v; }
    uint32_t get_rd_v  () const { return  rd_v; }
    int32_t  get_imm_v () const { return imm_v; }

    uint32_t get_PC      () const { return PC;     }
    uint32_t get_new_PC  () const { return new_PC; }

    uint32_t get_memory_addr() const { return memory_addr; }
    uint32_t get_memory_size() const { return memory_size; }

    // representation
    const std::string get_name() const { return name; }
    const std::string get_disasm() const;

    // executors
    void execute();
    void execute_unknown();

#define DECLARE_INSN(name, match, mask) \
void execute_ ## name ();
#include "opcodes.gen.h"
#undef DECLARE_INSN

    Executor function = &Instruction::execute_unknown;
};

#endif
