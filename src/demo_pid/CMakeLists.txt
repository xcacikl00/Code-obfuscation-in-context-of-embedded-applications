add_executable(PID_exe demo_pid.c ../shared/startup.c ../shared/utils.c)
target_include_directories(PID_exe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../shared")
