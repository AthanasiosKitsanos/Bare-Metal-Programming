.code32
.section .text
.global kernel_entry
.extern kernel_main
.extern _bss_start
.extern _bss_end
.extern _kernel_stack_top
.extern _mm_flag

.set DATA_SEG, 0x10

# DO NOT USE EBX UNTIL AFTER movb %bl, _mm_flag in the kernel_call label
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
    je .kernel_call
    movb $0, (%edi)
    incl %edi
    jmp .init_bss 

.kernel_call:
    movb %bl, _mm_flag
    movl %esp, %ebp
    call kernel_main

hang:
    cli
    hlt
    jmp hang
