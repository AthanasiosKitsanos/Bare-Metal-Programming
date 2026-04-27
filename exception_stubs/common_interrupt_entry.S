.code32
.section .text

.set DATA_SEG, 0x10

.global divide_error_stub
.global invalid_opcode_stub
.global common_interrupt_entry

.extern interrupt_dispatcher

divide_error_stub:
    pushl $0
    pushl $0
    jmp common_interrupt_entry

invalid_opcode_stub:
    pushl $0
    pushl $6
    jmp common_interrupt_entry

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
