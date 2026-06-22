.code32
.section .text
.global pm_entry
.extern kernel_main
.extern _bss_start
.extern _bss_end
.extern mm_flag

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

.leaf_1:
    movl $1, %eax
    cpuid

    andl $0x04000000, %edx
    cmpl $0x04000000, %edx
    jne .avx_done

    shrl $26, %edx
    pushl %edx
    xorl %ebx, %ebx
    andl $0x14000000, %ecx
    cmpl $0x14000000, %ecx
    jne .compute_mm_flag

    movl %cr4, %eax
    orl $0x00040000, %eax
    movl %eax, %cr4

.leaf_7:
    xorl %ecx, %ecx
    movl $0x7, %eax

    cpuid
    andl $0x00000020, %ebx
    cmpl $0x00000020, %ebx
    jne .compute_mm_flag

    xorl %ecx, %ecx
    xorl %edx, %edx
    movl $0x7, %eax
    xsetbv

.compute_mm_flag:
    popl %edx

    shrl $5, %ebx
    addl $1, %ebx
    imull %edx, %ebx
    
    movb %bl, mm_flag

.avx_done:
    movl %esp, %ebp
    call kernel_main

hang:
    cli
    hlt
    jmp hang
