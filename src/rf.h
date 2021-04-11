#ifndef RF_H
#define RF_H

#include "register.h"
#include "instruction.h"

class RF {
private:
    uint32_t read(Register num) const;
    void write(Register num, uint32_t value);

    struct RegisterState {
        uint32_t value = 0;
        bool is_valid = true; 
    };
    
    std::array<RegisterState, Register::MAX_NUMBER> register_table;
    
    void invalidate(Register num);
    bool is_valid(Register num) const;


    int32_t sign_extend(const int bits, uint32_t x) {
        uint32_t m = 1;
        m <<= bits - 1;
        return static_cast<int32_t>((x ^ m) - m);
    }
public:
    RF() { register_table[Register::zero()].is_valid = true; };
    
    void read_sources(Instruction &instr) const;
    void writeback(const Instruction &instr);
    void set_stack_pointer(uint32_t value);
    void validate(Register num);

    void dump() const;
};

#endif
