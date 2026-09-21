_interrupt_preamble_size = 52;
_kernel_stack_size = _kernel_raw_stack + _interrupt_preamble_size;
_interrupt_stack_size = _interrupt_raw_stack + (_interrupt_raw_stack + _interrupt_preamble_size) * 17;

ENTRY(kernel_entry)

SECTIONS
{
    . = 0x100000;
    _kernel_start = .;
    .text :
    {
        *kernel_entry.o(.text*)
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