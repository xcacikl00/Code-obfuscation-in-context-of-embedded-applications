
cmake_minimum_required(VERSION 3.14)

project(Obfuscator)


set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_C_COMPILER /usr/bin/clang)


# load config from LLVMConfig.cmake

#replace with installed version as necessary
find_package(LLVM 18.1.3 REQUIRED CONFIG)


include_directories(${LLVM_INCLUDE_DIRS})



add_library(Obfuscator MODULE 
    src/PassHandler.cpp 
    src/Flattening.cpp 
    src/Bogus.cpp 
    src/InstructionSubstitution.cpp 

)

target_include_directories(Obfuscator PRIVATE src)
