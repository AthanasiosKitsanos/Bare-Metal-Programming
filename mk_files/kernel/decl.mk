#--------------------Kernel Internals--------------------------
KERNEL_CPU_INTERRUPTS_H = include/kernel/internal/kernel_cpu_interrupts.h
KERNEL_HARDWARE_INTERRUPRS_H = include/kernel/internal/kernel_hardware_interrupts.h
KERNEL_CONTROL_INPUT_TABLE = include/kernel/internal/kernel_control_input_table.h
KERNEL_COMMAND_MAP = include/kernel/internal/kernel_command_functions.h

#---------------------Kernel Files--------------------------------
LOGGER_H = include/kernel/kernel_logger.h
LOGGER_CPP = src/kernel/kernel_logger.cpp
LOGGER_OBJ = obj/kernel/kernel_logger.o

ASSERT_H = include/kernel/kernel_assert.h
ASSERT_CPP = src/kernel/kernel_assert.cpp
ASSERT_OBJ = obj/kernel/kernel_assert.o

IDT_ENTRY_H = include/kernel/kernel_idt.h
IDT_ENTRY_CPP = src/kernel/kernel_idt.cpp
IDT_ENTRY_OBJ = obj/kernel/kernel_idt.o

EXCEPTIONS_H = include/kernel/kernel_exceptions.h
EXCEPTIONS_CPP = src/kernel/kernel_exceptions.cpp
EXCEPTIONS_OBJ = obj/kernel/kernel_exceptions.o

PIC_H = include/kernel/kernel_pic.h
PIC_CPP = src/kernel/kernel_pic.cpp
PIC_OBJ = obj/kernel/kernel_pic.o

PIT_H = include/kernel/kernel_pit.h
PIT_CPP = src/kernel/kernel_pit.cpp
PIT_OBJ = obj/kernel/kernel_pit.o

TIMER_H = include/kernel/kernel_timer.h
TIMER_CPP = src/kernel/kernel_timer.cpp
TIMER_OBJ = obj/kernel/kernel_timer.o

KERNEL_INTERRUPT_GUARD_H = include/kernel/kernel_interrupt_guard.h

#----------------------Include Folders--------------------------------------
INCLUDE_KERNEL_FOLDER = -Iinclude/kernel
INCLUDE_KERNEL_INTERNAL_FOLDER = -Iinclude/kernel/internal
INCLUDE_KERNEL_ALL_FOLDERS = $(INCLUDE_KERNEL_FOLDER) $(INCLUDE_KERNEL_INTERNAL_FOLDER)