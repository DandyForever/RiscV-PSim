#ifndef REGISTER_H
#define REGISTER_H

#include <array>

class Register {
public:
    enum class Names {
        zero,
        ra,
        sp,
        gp,
        tp,
        t0,
        t1, t2,
        s0,
        s1,
        a0, a1,
        a2, a3, a4, a5, a6, a7,
        s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
        t3, t4, t5, t6,
        MAX
    };

    friend std::ostream& operator<<(std::ostream& out, const Register& reg) {
        return out << reg.get_name();
    }

    const std::string get_name() const { return "$" + names_table[id()]; }

    Register(Names name) : name(name) { };
    Register(uint8_t number) : name(static_cast<Names>(number)) { };

    size_t id() const { return static_cast<size_t>(name); }
    static Register zero() { return Register(Names::zero); }
    operator size_t() const { return static_cast<size_t>(name); }

    static constexpr const size_t MAX_NUMBER = static_cast<size_t>(Names::MAX);
    static_assert(MAX_NUMBER == 32u, "Wrong number of registers");

private:
    Names name = Names::zero;
    static const std::array<std::string, MAX_NUMBER> names_table;
};

#endif
