#--------------------Kernel Internals--------------------------
KERNEL_CPU_INTERRUPTS_H = include/kernel/internal/kernel_cpu_interrupts.h
KERNEL_HARDWARE_INTERRUPRS_H = include/kernel/internal/kernel_hardware_interrupts.h
KERNEL_CONTROL_INPUT_TABLE = include/kernel/internal/kernel_control_input_table.h

#---------------------Kernel Files--------------------------------
LOGGER_H = include/kernel/kernel_logger.h

ASSERT_H = include/kernel/kernel_assert.h

IDT_ENTRY_H = include/kernel/kernel_idt.h

EXCEPTIONS_H = include/kernel/kernel_exceptions.h

PIC_H = include/kernel/kernel_pic.h

TIMER_H = include/kernel/kernel_timer.h

PIT_H = include/kernel/kernel_pit.h

INTERRUPT_GUARD_H = include/kernel/kernel_interrupt_guard.h

KERNEL_SHELL_H = include/kernel/kernel_shell.h

#----------------------Include Folders--------------------------------------
INCLUDE_KERNEL_FOLDER = -Iinclude/kernel
INCLUDE_KERNEL_INTERNAL_FOLDER = -Iinclude/kernel/internal
INCLUDE_KERNEL_ALL_FOLDERS = $(INCLUDE_KERNEL_FOLDER) $(INCLUDE_KERNEL_INTERNAL_FOLDER)