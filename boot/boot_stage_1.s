.code16
.section .text
.global _start

_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp
    cld
    sti

    movw $message, %si

print_loop:
    lodsb
    cmpb $0, %al
    je halt

    movb $0x0E, %ah
    movb $0x00, %bh
    movb $0x07, %bl
    int $0x10
    jmp print_loop

halt:
    cli

hang:
    hlt
    jmp hang

message:
    .asciz "Stage 1 bootloader works!"

.org 510
.word 0xAA55
