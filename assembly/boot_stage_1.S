.code16
.section .text
.global _start
.include "stage_2_sectors.inc"
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp
    cld
    sti

    movb %dl, boot_drive

    xorw %ax, %ax
    movw %ax, %es
    movw $0x7E00, %bx

    movb $0x02, %ah
    movb $BOOT_STAGE_2_SECTORS, %al
    movb $0x00, %ch
    movb $0x02, %cl
    movb $0x00, %dh
    movb boot_drive, %dl
    int $0x13
    jc disk_error

    ljmp $0x0000, $0x7E00

disk_error:
    movw $disk_error_message, %si
    call print_string
    cli

hang:
    hlt
    jmp hang

print_string:
    lodsb
    cmpb $0, %al
    je print_done

    movb $0x0E, %ah
    movb $0x00, %bh
    movb $0x07, %bl
    int $0x10
    jmp print_string

print_done:
    ret

boot_drive:
    .byte 0

disk_error_message:
    .asciz "Disk read error.\r\n"

.org 510
.word 0xAA55
