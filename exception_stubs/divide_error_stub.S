.code32
.section .text
.global invalid_opcode_stub
.extern invalid_opcode_exception_handler

invalid_opcode_stub:
    call invalid_opcode_exception_handler
