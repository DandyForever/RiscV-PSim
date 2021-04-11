#include "elf_manager.h"
#include "perfsim.h"
#include "funcsim.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Required arguments (1):FILE_NAME (2):NUM_CYCLES (3 optional):IS_FUNCTIONAL_SIMULATOR" << std::endl;
        return -1;
    }
    ElfManager elfManager(argv[1]);
    int num_cycles = atoi(argv[2]);
    int is_fsim = 0;
    if (argc == 4)
        is_fsim = atoi(argv[3]);

    if (is_fsim) {
        FuncSim simulator(elfManager.getWords(), elfManager.getPC());
        simulator.run(num_cycles);
    } else {
        PerfSim simulator(elfManager.getWords(), elfManager.getPC());
        simulator.run(num_cycles);
    }
    return 0;
}
