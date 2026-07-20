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
        _bss_start = .;
        *(COMMON)
        *(.bss*)
        _bss_end = .;
    }

    _stack_bottom = .;
    _interrupt_preamble_size = 52;
    . += (16384 + _interrupt_preamble_size);
    _stack_top = .;

    _interrupt_stack_bottom = .;
    . += 1024;
    _interrupt_stack_top = .;

    _kernel_end = .;
}