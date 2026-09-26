.code32
.section .text
.global pm_entry

.extern _sp_32_address
.extern kernel_entry
.extern _bss_start
.extern _kernel_start
.extern _kernel_size
.extern move_kernel

.set DATA_SEG, 0x10

pm_entry:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $_sp_32_address, %esp

.set_leaf_1:
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

    xorl %ebx, %ebx # Set EBX to zero
    andl $0x04000000, %edx
    jz .copy_kernel_setup

    shrl $26, %edx
    pushl %edx

    andl $0x14000000, %ecx
    cmpl $0x14000000, %ecx
    jne .compute_mm_flag

# .set_leaf_7:
#     movl %cr4, %eax
#     orl $0x00040000, %eax
#     movl %eax, %cr4

# .leaf_7:
#     xorl %ecx, %ecx
#     movl $0x7, %eax

#     cpuid
#     xorl %edx, %edx
#     andl $0x00000020, %ebx
#     jz .compute_mm_flag

#     xorl %ecx, %ecx
#     movl $0x7, %eax
#     xsetbv

#     xorl %ecx, %ecx
#     xgetbv
#     andl  $0x04, %eax
#     jnz .compute_mm_flag
#     xorl %ebx, %ebx

.compute_mm_flag:
    popl %edx

    shrl $5, %ebx
    addl $1, %ebx
    imull %edx, %ebx 

# DO NOT USE THE REGISTER EBX, IT HOLDS THE SIMD AND AVX FLAG

.copy_kernel_setup:
    cld
    movl $_kernel_source_image, %esi
    movl $_kernel_start, %edi
    movl $_kernel_size, %ecx
    jmp *.copy_kernel_method(,%ebx,4)

.copy_32:
    shrl $0x02, %ecx
    rep movsl (%esi), (%edi)
    jmp kernel_entry

.copy_sse2:
    movl %ecx, %edx
    addl %ecx, %esi
    addl %ecx, %edi
    negl %edx

.start_copy_sse_2:
    movdqa (%esi, %edx, 1), %xmm0
    movaps %xmm0, (%edi, %edx, 1)
    addl $0x10, %edx
    jnz .start_copy_sse_2
    jmp kernel_entry

.copy_avx2:
    movl %ecx, %edx
    addl %ecx, %esi
    addl %ecx, %edi
    negl %edx

.start_copy_avx_2:
    vmovdqa (%esi, %edx, 1), %ymm0
    vmovaps %ymm0, (%edi, %edx, 1)
    addl $0x20, %edx
    jnz .start_copy_avx_2
    vzeroupper
    jmp kernel_entry

hang:
    cli
    hlt
    jmp hang

.section .rodata
.balign 4
.copy_kernel_method:
    .long .copy_32
    .long .copy_sse2
    .long .copy_avx2