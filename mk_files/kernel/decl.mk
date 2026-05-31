#--------------------Kernel Internals--------------------------
KERNEL_CPU_INTERRUPTS_H = kernel/internal/kernel_cpu_interrupts.h
KERNEL_HARDWARE_INTERRUPRS_H = kernel/internal/kernel_hardware_interrupts.h
KERNEL_CONTROL_INPUT_TABLE = kernel/internal/kernel_control_input_table.h
KERNEL_COMMAND_MAP = kernel/internal/kernel_command_functions.h

#---------------------Kernel Files--------------------------------
# Logger
LOGGER_H = kernel/logger/kernel_logger.h
LOGGER_CPP = kernel/logger/kernel_logger.cpp
LOGGER_OBJ = obj/kernel/kernel_logger.o

# Assert
ASSERT_H = kernel/assert/kernel_assert.h
ASSERT_CPP = kernel/assert/kernel_assert.cpp
ASSERT_OBJ = obj/kernel/kernel_assert.o

# IDT
IDT_ENTRY_H = kernel/idt/kernel_idt.h
IDT_ENTRY_CPP = kernel/idt/kernel_idt.cpp
IDT_ENTRY_OBJ = obj/kernel/kernel_idt.o

# Exceptions
EXCEPTIONS_H = kernel/exceptions/kernel_exceptions.h
EXCEPTIONS_CPP = kernel/exceptions/kernel_exceptions.cpp
EXCEPTIONS_OBJ = obj/kernel/kernel_exceptions.o

# PIC
PIC_H = kernel/pic/kernel_pic.h
PIC_CPP = kernel/pic/kernel_pic.cpp
PIC_OBJ = obj/kernel/kernel_pic.o

#PIT
PIT_H = kernel/pit/kernel_pit.h
PIT_CPP = kernel/pit/kernel_pit.cpp
PIT_OBJ = obj/kernel/kernel_pit.o

# TIMER
TIMER_H = kernel/timer/kernel_timer.h
TIMER_CPP = kernel/timer/kernel_timer.cpp
TIMER_OBJ = obj/kernel/kernel_timer.o

#----------------------------------- Memories ------------------------------------------
# E820
MEMORY_E820_H = kernel/memory/e820/kernel_e820.h
MEMORY_E820_CPP = kernel/memory/e820/kernel_e820.cpp
MEMORY_E820_OBJ = obj/kernel/kernel_e820.o

# PMM
MEMORY_PMM_H = kernel/memory/pmm/kernel_pmm.h
MEMORY_PMM_CPP = kernel/memory/pmm/kernel_pmm.cpp
MEMORY_PMM_OBJ = obj/kernel/kernel_pmm.o

#--------------------------------Kernel Internal-------------------------------------
KERNEL_INTERRUPT_GUARD_H = kernel/internal/kernel_interrupt_guard.h

#----------------------Include Folders--------------------------------------
INCLUDE_KERNEL_FOLDER = -Ikernel