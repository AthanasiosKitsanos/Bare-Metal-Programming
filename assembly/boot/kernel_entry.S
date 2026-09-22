.code32
.section .text
.global kernel_entry
.extern kernel_main
.extern _bss_start
.extern _bss_end
.extern _kernel_stack_top
.extern mm_flag

.set DATA_SEG, 0x10

kernel_entry:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $_kernel_stack_top, %esp

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

.kernel_call:
    movl %esp, %ebp
    call kernel_main

hang:
    cli
    hlt
    jmp hang
