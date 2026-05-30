.code16
.section .text
.global _start_stage_2
.extern pm_entry

.set CODE_SEG, 0x08
.set DATA_SEG, 0x10

.set E820_MAGIC, 0x534D4150
.set E820_ENTRY_SIZE, 20
.set E820_COUNT_ADDRESS, 0x500
.set E820_ENTRIES_ADDRESS, 0x502

_start_stage_2:
    cli
    cld
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    movw $0x7A00, %sp

    call detect_memory

    lgdt gdt_descriptor

    movl %cr0, %eax
    orl $0x00000001, %eax
    movl %eax, %cr0

    ljmp $CODE_SEG, $pm_entry

hang:
    cli
    hlt
    jmp hang

detect_memory:
    novw $0, $(E820_COUNT_ADDRESS)

    xorw %ax, %ax
    movw %ax, %es

    movw $(E820_ENTRIES_ADDRESS), %di

    xorl %ebx, %ebx

.e820_loop:
    movl $0xE820, %%eax
    movl $20, %ecx
    movl $E820_MAGIC, %edx
    int 0x15
    jc .e820_done
    cmpl %eax, $E820_MAGIC
    jne .e820_done
    incw (E820_COUNT_ADDRESS)
    addw $E820_ENTRY_SIZE, %di
    testl %ebx, %ebx
    jz .e820_done
    jmp .e820_loop

.e820_done:
    ret

.align 8
gdt_start:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start
