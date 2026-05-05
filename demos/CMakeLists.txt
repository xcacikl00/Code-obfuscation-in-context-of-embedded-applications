cmake_minimum_required(VERSION 3.14)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

find_program(CLANG_PATH clang REQUIRED)
find_program(CLANGXX_PATH clang++ REQUIRED)

set(CMAKE_C_COMPILER ${CLANG_PATH})
set(CMAKE_CXX_COMPILER ${CLANGXX_PATH})
project(Demos)

# paths
set(SYSROOT "/usr/lib/arm-none-eabi")
set(GCC_LIB_PATH "/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard")
set(PLUGIN_PATH "${CMAKE_SOURCE_DIR}/../obfuscation_pass/build/libObfuscator.so")
set(SHARED_DIR "${CMAKE_SOURCE_DIR}/shared")

# flags
set(FLAGS 
    "--target=arm-none-eabi"
    "--sysroot=${SYSROOT}"
    "-mcpu=cortex-m4"
    "-mthumb"
    "-ffunction-sections"
    "-fdata-sections"
    "-DARM_MATH_DSP"
    "-mfloat-abi=hard"
    "-mfpu=fpv4-sp-d16"
    "-fpass-plugin=${PLUGIN_PATH}"
)

# convert list to string for compiler/linker
string(REPLACE ";" " " FLAGS_STR "${FLAGS}")



set(CMAKE_C_FLAGS "${FLAGS_STR}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -L${GCC_LIB_PATH} -L/usr/lib/arm-none-eabi/lib/thumb/v7e-m+fp/hard -lm -lgcc -lnosys -Wl,--gc-sections" CACHE STRING "" FORCE)


# set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -L${GCC_LIB_PATH} -lgcc -lnosys -Wl,--strip-all -Wl,--gc-sections" CACHE STRING "" FORCE)

add_subdirectory(demo_fft)
add_subdirectory(demo_aes)
add_subdirectory(demo_pid)


