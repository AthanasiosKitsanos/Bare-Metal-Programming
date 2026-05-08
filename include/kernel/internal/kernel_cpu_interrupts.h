#pragma once

#define CPU_INTERRUPT_LIST  \
    X(0, divide_error, "Divide Error", "#DE")   \
    X(1, u1, "Unitialize CPU exception", "#UCE")   \
    X(2, u2, "Unitialize CPU exception", "#UCE")   \
    X(3, u3, "Unitialize CPU exception", "#UCE")   \
    X(4, u4, "Unitialize CPU exception", "#UCE")   \
    X(5, u5, "Unitialize CPU exception", "#UCE")   \
    X(6, invalid_opcode, "Invalid Opcode", "#UD")   \
    X(7, u7, "Unitialize CPU exception", "#UCE")   \
    X(8, u8, "Unitialize CPU exception", "#UCE")   \
    X(9, u9, "Unitialize CPU exception", "#UCE")   \
    X(10, u10, "Unitialize CPU exception", "#UCE")   \
    X(11, u11, "Unitialize CPU exception", "#UCE")   \
    X(12, u12, "Unitialize CPU exception", "#UCE")   \
    X(13, general_protection_fault, "General Protection Fault", "#GP")   \
    X(14, page_fault, "Page Fault", "#PF")  \
    X(15, u15, "Unitialize CPU exception", "#UCE")   \
    X(16, u16, "Unitialize CPU exception", "#UCE")   \
    X(17, u17, "Unitialize CPU exception", "#UCE")   \
    X(18, u18, "Unitialize CPU exception", "#UCE")   \
    X(19, u19, "Unitialize CPU exception", "#UCE")   \
    X(20, u20, "Unitialize CPU exception", "#UCE")   \
    X(21, u21, "Unitialize CPU exception", "#UCE")   \
    X(22, u22, "Unitialize CPU exception", "#UCE")   \
    X(23, u23, "Unitialize CPU exception", "#UCE")   \
    X(24, u24, "Unitialize CPU exception", "#UCE")   \
    X(25, u25, "Unitialize CPU exception", "#UCE")   \
    X(26, u26, "Unitialize CPU exception", "#UCE")   \
    X(27, u27, "Unitialize CPU exception", "#UCE")   \
    X(28, u28, "Unitialize CPU exception", "#UCE")   \
    X(29, u29, "Unitialize CPU exception", "#UCE")   \
    X(30, u30, "Unitialize CPU exception", "#UCE")   \
    X(31, u31, "Unitialize CPU exception", "#UCE")   
