#----------------------------Kernel Source Files------------------------------------------------
LOGGER_CPP = src/kernel/kernel_logger.cpp
LOGGER_OBJ = obj/kernel/kernel_logger.o

ASSERT_CPP = src/kernel/kernel_assert.cpp
ASSERT_OBJ = obj/kernel/kernel_assert.o

IDT_ENTRY_CPP = src/kernel/kernel_idt.cpp
IDT_ENTRY_OBJ = obj/kernel/kernel_idt.o

EXCEPTIONS_CPP = src/kernel/kernel_exceptions.cpp
EXCEPTIONS_OBJ = obj/kernel/kernel_exceptions.o

TIMER_CPP = src/kernel/kernel_timer.cpp
TIMER_OBJ = obj/kernel/kernel_timer.o

PIC_CPP = src/kernel/kernel_pic.cpp
PIC_OBJ = obj/kernel/kernel_pic.o

PIT_CPP = src/kernel/kernel_pit.cpp
PIT_OBJ = obj/kernel/kernel_pit.o