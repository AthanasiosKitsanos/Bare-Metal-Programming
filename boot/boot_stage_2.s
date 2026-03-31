.code16
.section .text
.global _start

_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    cld
    sti

    movw $message_stage_2, %si
    call print_string

halt:
    cli

hang:
    hlt
    jmp hang

print_string:
    lodsb
    cmpb $0, %al
    je done

    movb $0x0E, %ah
    movb $0x00, %bh
    movb $0x07, %bl
    int $0x10
    jmp print_string

done:
    ret

message_stage_2:
    .asciz "Stage 2 is running from a reserved 4-sector region...\r\n"
