_sector_size = 512;

ENTRY(_start_stage_2)

SECTIONS
{
    . = 0x7E00;
    _sp_32_address = .;
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

    . = ALIGN(_sector_size);
    _kernel_source_image = .;

    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
}