.code16
.section .text
.global _start_stage_2
.extern pm_entry

.set CODE_SEG, 0x08
.set DATA_SEG, 0x10

_start_stage_2:
    cli
    cld
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    movw $0x7A00, %sp

    lgdt gdt_descriptor

    movl %cr0, %eax
    orl $0x00000001, %eax
    movl %eax, %cr0

    ljmp $CODE_SEG, $pm_entry

hang:
    cli
    hlt
    jmp hang

.align 8
gdt_start:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start
