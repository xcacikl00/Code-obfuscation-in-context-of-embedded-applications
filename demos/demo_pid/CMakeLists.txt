add_executable(PID.elf demo_pid.c ../shared/startup.c ../shared/utils.c)
target_link_options(PID.elf PRIVATE "-T" "${SHARED_DIR}/linker.ld")

target_include_directories(PID.elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../shared")
