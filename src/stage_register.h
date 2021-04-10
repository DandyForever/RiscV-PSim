#ifndef _PORT_H_
#define _PORT_H_

#include "instruction.h"

template <class Data>
class StageRegister {
private:
    Data* data_in = nullptr;
    Data* data_out = nullptr;
public:
    void clock() { data_out = data_in; }
    void write(Data* input) { data_in = input; }
    Data* read() { return data_out; }
};

#endif
