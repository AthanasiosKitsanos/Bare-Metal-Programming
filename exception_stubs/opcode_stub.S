.code32
.section .text
.global divide_error_stub
.extern zero_division_exception_handler

divide_error_stub:
    call divide_error_exception_handler

halt_forever:
    cli
    hlt
    jmp halt_forever
