.code32
.section .text
.global pm_entry
.extern kernel_main
.extern _bss_start
.extern _bss_end

.set DATA_SEG, 0x10

pm_entry:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $0x90000, %esp

    movl $_bss_start, %edi
    movl $_bss_end, %ecx

.init_bss:
    cmpl %edi, %ecx
    je .init_end
    movb $0, (%edi)
    incl %edi
    jmp .init_bss

.init_end:
    movl %cr0, %eax
    andl $0xFFFFFFFB, %eax
    orl $0x00000002, %eax
    movl %eax, %cr0

    movl %cr4, %eax
    orl $0x00000600, %eax
    movl %eax, %cr4 

    movl %esp, %ebp

    call kernel_main

hang:
    cli
    hlt
    jmp hang
