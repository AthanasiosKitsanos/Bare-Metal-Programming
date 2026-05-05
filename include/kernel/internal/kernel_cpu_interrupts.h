#pragma once

#define CPU_INTERRUPT_LIST  \
    X(0, divide_error, "Divide Error", "#DE")   \
    X(6, invalid_opcode, "Invalid Opcode", "#UD")   \
    X(13, general_protection_fault, "General Protection Fault", "#GP")   \
    X(14, page_fault, "Page Fault", "#PF")
