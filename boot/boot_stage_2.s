.code16
.section .text
.global _start_stage_2

_start_stage_2:
    cli
    cld
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    movw $0x7A00, %sp

hang:
    hlt
    jmp hang
