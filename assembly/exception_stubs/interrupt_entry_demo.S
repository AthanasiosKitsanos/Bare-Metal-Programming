.code32
.section .bss
.global pic_counter
.global exception_counter

pic_counter:
    .zero 1
exception_counter:
    .zero 1

.section .text
.extern _interrupt_stack_top
.extern _interrupt_stack_bottom
.extern _stack_top

.set exception_count_max, 0x03
.set pic_count_max, 0x0F
.set DATA_SEG, 0x10

.global common_interrupt_entry

.extern interrupt_dispatcher

.macro ISR_NO_ERROR vector
.global isr_\vector
isr_\vector:
    cmpb $exception_count_max, exception_counter
    jae nesting_overflow_hlt
    incb exception_counter
    pushl $0
    pushl $\vector
    pusha
    jmp common_interrupt_entry
.endm

.macro ISR_WITH_ERROR vector
.global isr_\vector
isr_\vector:
    cmpb $exception_count_max, exception_counter
    jae nesting_overflow_hlt
    incb exception_counter
    pushl $\vector
    pusha
    jmp common_interrupt_entry
.endm

.macro IRQ vector
.global irq_\vector
irq_\vector:
    cmpb $pic_count_max, pic_counter
    jae nesting_overflow_hlt
    incb pic_counter
    pushl $0
    pushl $\vector
    pusha
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
    movl %esp, %edx
    cmpl $_interrupt_stack_bottom, %esp
    jna set_esp_to_int_stack
    cmpl $_interrupt_stack_top, %esp
    jnbe set_esp_to_int_stack
    jmp int_stack_set

set_esp_to_int_stack:
    movl $_interrupt_stack_top, %esp

int_stack_set:
    pushl %edx

set_interrupt_grounds:
    movw $DATA_SEG, %ax
    movw %ax, %ds
    movw %ax, %es

    pushl %edx
    call interrupt_dispatcher
    addl $4, %esp

    popl %esp
    cmpl $0x20, 0x20(%esp)
    jae decrement_pic_coutner
    decb exception_counter
    jmp end_of_interrupt

decrement_pic_coutner:
    decb pic_counter

end_of_interrupt:
    popa
    addl $8, %esp
    iret

nesting_overflow_hlt:
    cli

halt_interrupt_table:
    hlt
    jmp halt_interrupt_table