OUTPUT_FORMAT("a.out-pdp11")
ENTRY(_start)

bss_start = 040;
bss_end = 0320;
load_address = 01000;
ram_size = 040000;

MEMORY
{
    BSS (rw)    : ORIGIN = bss_start, LENGTH = bss_end - bss_start
    RAM (rwx)   : ORIGIN = load_address, LENGTH = ram_size - load_address
}

SECTIONS
{
    .bss ALIGN(2) :
    {
        *(.bss)
    } >BSS

    .text :
    {
        *(.text)
    } >RAM

    .data ALIGN(2) :
    {
        *(.data)
    } >RAM
}
