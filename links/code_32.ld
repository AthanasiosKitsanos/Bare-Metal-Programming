_interrupt_preamble_size = 52;
_kernel_stack_size = _kernel_raw_stack + _interrupt_preamble_size;
_interrupt_stack_size = _interrupt_raw_stack + (_interrupt_raw_stack + _interrupt_preamble_size) * 17;

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

    _kernel_stack_bottom = .;
    . += _kernel_stack_size;
    _kernel_stack_top = .;

    _interrupt_stack_bottom = .;
    . += _interrupt_stack_size;
    _interrupt_stack_top = .;

    _kernel_end = .;
}