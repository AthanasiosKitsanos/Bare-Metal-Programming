.code32
.section .text
.extern _interrupt_stack_top
.extern _stack_top

.set DATA_SEG, 0x10

.global common_interrupt_entry

.extern interrupt_dispatcher

.macro ISR_NO_ERROR vector
.global isr_\vector
isr_\vector:
    call check_interrupt_stack

    pushl $0
    pushl $\vector
    jmp common_interrupt_entry
.endm

.macro ISR_WITH_ERROR vector
.global isr_\vector
isr_\vector:
    call check_interrupt_stack

    pushl $\vector
    jmp common_interrupt_entry
.endm

.macro IRQ vector
.global irq_\vector
irq_\vector:
    call check_interrupt_stack

    pushl $0
    pushl $\vector
    jmp common_interrupt_entry
.endm

.irp v, 0, 1, 2, 3, 4, 5, 6, 7, 9, 15, 16
ISR_NO_ERROR \v
.endr

.irp v, 8, 10, 11, 12, 13, 14, 17
    ISR_WITH_ERROR \v
.endr

.irp v, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    ISR_NO_ERROR \v
.endr

.irp v, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
    IRQ \v
.endr

common_interrupt_entry:
    pusha

    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es

    pushl %esp
    call interrupt_dispatcher
    addl $4, %esp

    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es

    popa
    addl $8, %esp
    iret

check_interrupt_stack:
    popl %eax
    cmpl $_interrupt_stack_bottom, %esp
    jna go_to_interrupt_stack
    cmpl $_interrupt_stack_top, %esp
    jnbe go_to_interrupt_stack
    jmp interrupt_stack_set

go_to_interrupt_stack:
    movl $_interrupt_stack_top, %esp

interrupt_stack_set:
    jmp *%eax