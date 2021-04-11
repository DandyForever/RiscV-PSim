#include "rf.h"
#include "consts.h"

uint32_t RF::read(Register num) const {
    if (is_valid(num))
        return register_table[num].value;
    
    throw std::invalid_argument("Register " + num.get_name() + " is INVALID");
}

void RF::write(Register num, uint32_t value) {
    if (num == 0) return;
    register_table[num].value = value;
    validate(num);
}

void RF::invalidate(Register num) {
    if (num == 0) return;
    register_table[num].is_valid = false;
}

void RF::validate(Register num) {
    register_table[num].is_valid = true;
}

bool RF::is_valid(Register num) const {
    return register_table[num].is_valid;
}


int32_t RF::sign_extend(const int bits, uint32_t x) {
    uint32_t m = 1;
    m <<= bits - 1;
    return static_cast<int32_t>((x ^ m) - m);
}

void RF::read_sources(Instruction &instr) const {
    instr.set_rs1_v(read(instr.get_rs1()));
    instr.set_rs2_v(read(instr.get_rs2()));
}

void RF::writeback(const Instruction &instr) {
    uint32_t value = instr.get_rd_v();

    if (instr.is_sign_extended_load()) {
        auto bits = 8 * instr.get_memory_size();
        int32_t sign_extended_value = sign_extend(bits, value);
        value = static_cast<uint32_t>(sign_extended_value);
    }

    write(instr.get_rd(), value);
}

void RF::set_stack_pointer(uint32_t value) {
    write(Register::Names::sp, value);
}

void RF::dump() const {
    if (!IS_DUMP_RF)
        return;

    std::cout << "Register file dump:" << std::endl;
    
    for(uint8_t i = 0; i < (Register::MAX_NUMBER); i++) {
        if (!is_valid(i))
            continue;

        std::cout << '\t' << Register(i) << " = " << std::hex << register_table[i].value << std::endl;
    }

    std::cout << std::endl;
}
