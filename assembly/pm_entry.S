.code32
.section .text
.global pm_entry

.extern _bss_start
.extern _kernel_start
.extern kernel_entry

.set DATA_SEG, 0x10

pm_entry:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    cld
    movl $_kernel_source_image, %esi
    movl $_kernel_start, %edi

    movl $_bss_start, %eax      # Load the address of _bss_start
    subl $_kernel_start, %eax   # Substruct the address of _kernel_start to get the byte difference
    movl %eax, %ecx             # move the result to %ecx, since it is needed for rep movsl

    shrl $0x02, %ecx            # find how many dwords fit in that distance
    rep movsl (%esi), (%edi)    # copy from one address to another

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
    jz .kernel_entry_jump

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
.kernel_entry_jump:
    jmp kernel_entry

hang:
    cli
    hlt
    jmp hang
