cmake_minimum_required(VERSION 3.16)
project(psim)

set(CMAKE_CXX_STANDARD 14)

add_executable(psim main.cpp cache.cpp cache.h types.h common.cpp common.h config.cpp config.h elf.cpp elf.h funcsim.cpp funcsim.h register.cpp register.h decoder.h opcodes.gen.h instruction.cpp instruction.h execute.cpp memory.cpp memory.h perfsim.cpp perfsim.h rf.cpp rf.h stage_register.h)