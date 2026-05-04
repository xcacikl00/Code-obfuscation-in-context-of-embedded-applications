ENTRY(Reset_Handler)  

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K

}

SECTIONS
{
    .text : {
        KEEP(*(.vector_table)) 
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .; 
    } > FLASH

    _la_data = LOADADDR(.data);

    .data : {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

.bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    

    _stack_top = ALIGN(ORIGIN(RAM) + LENGTH(RAM), 8);
    _end = .;
    __end__ = .;
}