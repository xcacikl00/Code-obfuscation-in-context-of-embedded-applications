cmake_minimum_required(VERSION 3.28)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_SYSROOT "/usr/lib/arm-none-eabi")
set(LIBGCC_PATH "/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard")

set(TARGET_FLAGS "--target=arm-none-eabi -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard  --sysroot=${ARM_SYSROOT}")

set(CMAKE_C_FLAGS "${TARGET_FLAGS} -O0 -g -fno-omit-frame-pointer -ffunction-sections -fdata-sections")

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/shared/linker.ld")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS} -T ${LINKER_SCRIPT} -L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7e-m+fp/hard -L/usr/lib/arm-none-eabi/lib/thumb/v7e-m+fp/hard -lm -fuse-ld=lld -Wl,--gc-sections -lgcc -nostdlib")
project(Demos)

set(CHOSEN_DEMO "AES" CACHE STRING "Choose Demo AES/PID/FFT")

if(CHOSEN_DEMO STREQUAL "AES")
    add_subdirectory(demo_aes)
elseif(CHOSEN_DEMO STREQUAL "FFT")

    add_subdirectory(demo_fft)
elseif(CHOSEN_DEMO STREQUAL "PID")
    add_subdirectory(demo_pid)
endif()

add_custom_target(run
    COMMAND qemu-system-arm -M olimex-stm32-h405 -nographic -kernel $<TARGET_FILE:${CHOSEN_DEMO}_exe>
    DEPENDS ${CHOSEN_DEMO}_exe
    COMMENT "Running ${CHOSEN_DEMO} in QEMU (GDB Server on 1234)..."
)