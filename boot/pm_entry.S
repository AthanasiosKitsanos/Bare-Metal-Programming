.code32
.section .text
.global pm_entry
.extern kernel_main

.set DATA_SEG, 0x10

pm_entry:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $0x90000, %esp
    movl %esp, %ebp

    call kernel_main

hang:
    cli
    hlt
    jmp hang
