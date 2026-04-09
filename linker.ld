MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    .text : {
        KEEP(*(.vector_table))
        *(.text*)
        *(.rodata*)
        _etext = .; 
    } > FLASH

    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    .bss : {
        __bss_start__ = .;  /* Required by newlib crt0 */
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;    /* Required by newlib crt0 */
    } > RAM

    /* Required for sbrk (malloc/heap) */
    . = ALIGN(8);
    end = .;                
    __end__ = .;           
    _end = .;
}