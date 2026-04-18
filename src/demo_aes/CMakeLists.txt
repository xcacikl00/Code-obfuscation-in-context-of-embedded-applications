add_executable(AES_exe 
    demo_aes.c
    ../shared/startup.c
    ../shared/utils.c
    tiny-AES-C/aes.c
)

target_include_directories(AES_exe PRIVATE 
    tiny-AES-C 
    ../shared
)

target_compile_definitions(AES_exe PRIVATE CBC=1 ECB=1)67