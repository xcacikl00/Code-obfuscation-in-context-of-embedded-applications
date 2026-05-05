cmake_minimum_required(VERSION 3.14)
project(FFT_Demo)

set(CMSISDSP "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DSP")

# needed for CMSIS DSP to find the core headers
set(CMSISCORE "${CMAKE_CURRENT_SOURCE_DIR}/CMSIS_6/CMSIS/Core")

# call CMSIS DSP CMakeLists.txt to build the library
add_subdirectory("${CMSISDSP}/Source" bin_dsp)

add_executable(FFT.elf demo_fft.c ../shared/startup.c ../shared/utils.c)

# link against the CMSIS DSP library, while building the whole library is somewhat innfeficien, it is the simplest way to resolve all dependencies.
# unneeded functions will be trimmed by the linker
target_link_libraries(FFT.elf PRIVATE CMSISDSP )
target_link_options(FFT.elf PRIVATE "-T" "${SHARED_DIR}/linker.ld")

target_include_directories(FFT.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../shared")

target_compile_definitions(FFT.elf PRIVATE 
    ARM_MATH_CM4 
    __FPU_PRESENT=1
    

)