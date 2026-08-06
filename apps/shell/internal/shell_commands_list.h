#pragma once

#define COMMAND_LIST    \
    X(0, "clear")   \
    X(1, "exit")    \
    X(2, "flag")   \
    X(3, "interrupt stack") \
    X(4, "kernel stack")    \
    X(5, "ticks")   

#define COMMAND_FUNCTIONS    \
    X(0, clear) \
    X(1, exit)    \
    X(2, flag)  \
    X(3, interrupt_stack)   \
    X(4, kernel_stack)  \
    X(5, ticks)
