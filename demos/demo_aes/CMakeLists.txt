add_executable(AES.elf 
    demo_aes.c
    ../shared/startup.c
    ../shared/utils.c
    tiny-AES-C/aes.c
)

target_include_directories(AES.elf PRIVATE 
    tiny-AES-C 
    ../shared
)

target_link_options(AES.elf PRIVATE "-T" "${SHARED_DIR}/linker.ld")


target_compile_definitions(AES.elf PRIVATE CBC=1 ECB=1)