#---------------------Kernel Files--------------------------------
# Logger
LOGGER_H = kernel/logger.h
LOGGER_CPP = kernel/logger.cpp
LOGGER_OBJ = obj/kernel/logger.o

# IDT
IDT_ENTRY_H = kernel/idt.h
IDT_ENTRY_CPP = kernel/idt.cpp
IDT_ENTRY_OBJ = obj/kernel/idt.o

# Exceptions
EXCEPTIONS_H = kernel/exceptions.h
EXCEPTIONS_CPP = kernel/exceptions.cpp
EXCEPTIONS_OBJ = obj/kernel/exceptions.o

# PIC
PIC_H = kernel/pic.h
PIC_CPP = kernel/pic.cpp
PIC_OBJ = obj/kernel/pic.o

#PIT
PIT_H = kernel/pit.h
PIT_CPP = kernel/pit.cpp
PIT_OBJ = obj/kernel/pit.o

# TIMER
TIMER_H = kernel/timer.h
TIMER_CPP = kernel/timer.cpp
TIMER_OBJ = obj/kernel/timer.o

#----------------------------------- Memories ------------------------------------------
# E820
MEMORY_E820_H = kernel/e820.h
MEMORY_E820_CPP = kernel/e820.cpp
MEMORY_E820_OBJ = obj/kernel/e820.o

# PMM
MEMORY_PMM_H = kernel/pmm.h
MEMORY_PMM_CPP = kernel/pmm.cpp
MEMORY_PMM_OBJ = obj/kernel/pmm.o