.code32
.section .text

.global invalid_opcode_stub
.global divide_error_stub

.extern invalid_opcode_exception_handler
.extern divide_error_exception_handler

invalid_opcode_stub:
    call invalid_opcode_exception_handler

divide_error_stub:
    call divide_error_exception_handler

halt_forever:
    cli
    hlt
    jmp halt_forever
