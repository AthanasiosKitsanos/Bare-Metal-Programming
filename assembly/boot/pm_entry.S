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

    andl $0x03, %eax            # find the remaning bytes that are left
    movl %eax, %ecx             # move left bytes to %ecx
    rep movsb (%esi), (%edi)    # copy the rest of the bytes

    jmp kernel_entry

hang:
    cli
    hlt
    jmp hang
