.code32
.section .text
.global invalid_division_stub
.extern zero_division_exception_handler

invalid_division_stub:
    call zero_division_exception_handler
