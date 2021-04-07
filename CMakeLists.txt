cmake_minimum_required(VERSION 3.16)
project(psim)

set(CMAKE_CXX_STANDARD 17)

add_executable(psim main.cpp cache.cpp cache.h types.h common.cpp common.h config.cpp config.h elf_manager.cpp elf_manager.h functional_simulator.cpp functional_simulator.h register.cpp register.h decoder.h opcodes.gen.h instruction.cpp instruction.h execute.cpp memory.cpp memory.h perfsim.cpp perfsim.h rf.cpp rf.h stage_register.h)
