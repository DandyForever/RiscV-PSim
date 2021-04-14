cmake_minimum_required(VERSION 3.16)
project(psim)

find_package(Boost QUIET REQUIRED COMPONENTS program_options)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost Not Found")
endif()

find_library(LIBELF_LIBRARY elf)
if(NOT LIBELF_LIBRARY)
    message(SEND_ERROR "Failed to find libelf")
    return()
endif()

set(CMAKE_CXX_STANDARD 17)

add_executable(psim main.cpp cache.cpp cache.h elf_manager.cpp elf_manager.h funcsim.cpp funcsim.h register.cpp register.h decoder.h opcodes.gen.h instruction.cpp instruction.h execute.cpp memory.cpp memory.h latch.h perfsim.cpp perfsim.h rf.cpp rf.h hazard_unit.cpp hazard_unit.h mmu.cpp mmu.h)
    
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::program_options ${LIBELF_LIBRARY} )
