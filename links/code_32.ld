ENTRY(_start_stage_2)

SECTIONS
{
    . = 0x7E00;
    _kernel_start = .;
    .text :
    {
        *(.text*)
    }
    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
    _kernel_end = .;
}