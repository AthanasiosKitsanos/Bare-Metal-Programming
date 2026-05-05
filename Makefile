# ----------------Compiling Configuration--------------------
CC = i686-elf-g++

AS = i686-elf-as
LD = i686-elf-ld
OBJC = i686-elf-objcopy

MAKE_LIB = i686-elf-ar rcs

QEMU = qemu-system-x86_64

COMPILE_FLAGS = -std=gnu++17 -ffreestanding -O3 -Wall -Wextra -fno-exceptions -fno-rtti
LINKING_FLAGS = -ffreestanding -O3 -nostdlib

SECTOR_SIZE = 512

INCLUDE_MAP_FILE = -Map=output.map

# File Sources

# ---------------------Boot stage 1---------------------------
BOOT_STAGE_1 = boot/boot_stage_1.S
BOOT_STAGE_1_OBJ = obj/boot_stage_1.o
BOOT_STAGE_1_ELF = elf/boot_stage_1.elf
BOOT_STAGE_1_BIN = bin/boot_stage_1.bin
BOOT_1_LIKNER = links/boot_1_linker.ld

# ---------------------Boot stage 2---------------------------
BOOT_STAGE_2_INC = inc/boot_stage_2_load.inc
BOOT_STAGE_2 = boot/boot_stage_2.S
BOOT_STAGE_2_OBJ = obj/boot_stage_2.o

# --------------------Code 32--------------------------------
CODE_32_ELF = elf/code_32.elf
CODE_32_BIN = bin/code_32.bin
CODE_32_LINKER = links/code_32.ld


# -----------------------Kenrel-------------------------------
KERNEL_CPP = kernel/kernel.cpp
KERNEL_OBJ = obj/kernel.o

IO_H = include/terminal/terminal_io_registers.h

TERMINAL_H = include/terminal/terminal.h
TERMINAL_CPP = src/terminal/terminal.cpp
TERMINAL_OBJ = obj/terminal/terminal.o

VGA_H = include/terminal/terminal_vga_text_buffer.h
VGA_CPP = src/terminal/terminal_vga_text_buffer.cpp
VGA_OBJ = obj/terminal/terminal_vga_text_buffer.o

CURSOR_H = include/terminal/terminal_vga_hardware_cursor.h
CURSOR_CPP = src/terminal/terminal_vga_hardware_cursor.cpp
CURSOR_OBJ = obj/terminal/terminal_vga_hardware_cursor.o

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

TIMER_H = include/kernel/kernel_timer.h
TIMER_CPP = src/kernel/kernel_timer.cpp
TIMER_OBJ = obj/kernel/kernel_timer.o

PIT_H = include/kernel/kernel_pit.h
PIT_CPP = src/kernel/kernel_pit.cpp
PIT_OBJ = obj/kernel_pit.o

KEYBOARD_KEY_LIST_H = include/kernel/internal/kernel_keyboard_key_list.h

KEYBOARD_H = include/kernel/keyboard.h
KEYBOARD_CPP = src/kernel/keyboard.cpp
KEYBOARD_OBJ = obj/kernel/keyboard.o

# --------------------Include Folders--------------------------
INCLUDE_TERMINAL_FOLDER = -Iinclude/terminal
INCLUDE_KERNEL_FOLDER = -Iinclude/kernel
INCLUDE_KERNEL_INTERNALS = -Iinclude/kernel/internal
INCLUDE_FOLDERS = $(INCLUDE_TERMINAL_FOLDER) $(INCLUDE_KERNEL_FOLDER)

# ------------------------Pm Entry---------------------------
PM_ENTRY = boot/pm_entry.S
PM_ENTRY_OBJ = obj/pm_entry.o

#-------------------------Common Interrupt Entry-------------------------------
INTERRUPT_ENTRY_S = exception_stubs/common_interrupt_entry.S
INTERRUPT_ENTRY_OBJ = obj/exception_stubs/commom_interrupt_entry.o

# ------------------------Library----------------------------
KERNEL_A = lib/libkernel.a
LINK_LIBS = -Llib -lkernel
LIB_FILES = $(KERNEL_OBJ) $(VGA_OBJ) $(TERMINAL_OBJ) $(CURSOR_OBJ) $(LOGGER_OBJ) $(ASSERT_OBJ) $(IDT_ENTRY_OBJ) $(EXCEPTIONS_OBJ) $(PIC_OBJ) $(TIMER_OBJ) $(PIT_OBJ) $(KEYBOARD_OBJ)

# ------------------------OS Image---------------------------
OS_IMAGE = bin/os_image.bin

# ----------------------Rules--------------------------------

all: $(OS_IMAGE)

# Kernel
$(KERNEL_OBJ): $(KERNEL_CPP) $(TERMINAL_H) $(VGA_H) $(IO_H) $(CURSOR_H) $(LOGGER_H) $(ASSERT_H) $(EXCEPTIONS_H) $(TIMER_H) $(PIT_H) $(KEYBOARD_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(KERNEL_CPP) -o $(KERNEL_OBJ)

# VGA cursor
$(CURSOR_OBJ): $(IO_H) $(CURSOR_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_TERMINAL_FOLDER) -c $(CURSOR_CPP) -o $(CURSOR_OBJ)

# VGA Buffer
$(VGA_OBJ): $(VGA_CPP) $(VGA_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_TERMINAL_FOLDER) -c $(VGA_CPP) -o $(VGA_OBJ)

# Terminal
$(TERMINAL_OBJ): $(VGA_H) $(TERMINAL_H) $(TERMINAL_CPP) $(IO_H) $(LOGGER_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(TERMINAL_CPP) -o $(TERMINAL_OBJ)

# Kernel Logger
$(LOGGER_OBJ): $(LOGGER_H) $(LOGGER_CPP) $(TERMINAL_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(LOGGER_CPP) -o $(LOGGER_OBJ)

# Kernel Assert
$(ASSERT_OBJ): $(ASSERT_CPP) $(ASSERT_H) $(LOGGER_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(ASSERT_CPP) -o $(ASSERT_OBJ)

# Kenrel IDT
$(IDT_ENTRY_OBJ): $(IDT_ENTRY_CPP) $(IDT_ENTRY_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_KERNEL_FOLDER) -c $(IDT_ENTRY_CPP) -o $(IDT_ENTRY_OBJ)

# Kernel Timer
$(TIMER_OBJ): $(TIMER_CPP) $(TIMER_H) $(LOGGER_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(TIMER_CPP) -o $(TIMER_OBJ)

# Kernel Exceptions
$(EXCEPTIONS_OBJ): $(EXCEPTIONS_CPP) $(EXCEPTIONS_H) $(LOGGER_H) $(IDT_ENTRY_H) $(INTERRUPT_FRAME_H) $(PIC_H) $(TIMER_H) $(PIT_H) $(KEYBOARD_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(EXCEPTIONS_CPP) -o $(EXCEPTIONS_OBJ)

# Kernel PIC
$(PIC_OBJ): $(PIC_CPP) $(PIC_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(PIC_CPP) -o $(PIC_OBJ)

# Kernel PIT
$(PIT_OBJ): $(PIT_CPP) $(PIT_H) $(IO_H) $(LOGGER_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) -c $(PIT_CPP) -o $(PIT_OBJ)

# Keyboard
$(KEYBOARD_OBJ): $(KEYBOARD_CPP) $(KEYBOARD_H) $(IO_H) $(KEYBOARD_KEY_LIST_H)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FOLDERS) $(INCLUDE_KERNEL_INTERNALS) -c $(KEYBOARD_CPP) -o $(KEYBOARD_OBJ)

# KERNEL_A
$(KERNEL_A): $(LIB_FILES)
	$(MAKE_LIB) $(KERNEL_A) $(LIB_FILES)

# PM Entry
$(PM_ENTRY_OBJ): $(PM_ENTRY)
	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# Common Interrupt Entry
$(INTERRUPT_ENTRY_OBJ): $(INTERRUPT_ENTRY_S) 
	$(AS) $(INTERRUPT_ENTRY_S) -o $(INTERRUPT_ENTRY_OBJ)

# Boot 2
$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

# Code 32
$(CODE_32_ELF): $(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_A)
	$(LD) -T $(CODE_32_LINKER) -o $(CODE_32_ELF) \
		$(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(LINK_LIBS)

$(CODE_32_BIN): $(CODE_32_ELF)
	$(OBJC) -O binary $(CODE_32_ELF) $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN) ); \
	sectors=$$(( ($$size + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
	padded_size=$$(( $$sectors * $(SECTOR_SIZE) )); \
	if [ $$sectors -gt 127 ]; then \
		echo "Stage 2 is too large: $$sectors sectors (BIOS read limit exceeded)."; \
		exit 1; \
	fi; \
	truncate -s $$padded_size $(CODE_32_BIN)

# Loading Configuration
$(BOOT_STAGE_2_INC): $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN)); \
	sectors=$$(( $$size / $(SECTOR_SIZE) )); \
	if [ -z "$$sectors" ]; then \
		echo "Failed to compute stage 2 sector count."; \
		exit 1; \
	fi; \
	printf '.set BOOT_STAGE_2_SECTORS, %s\n' "$$sectors" > $(BOOT_STAGE_2_INC)

# Boot 1
$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1) $(BOOT_STAGE_2_INC)
	$(AS) -I inc $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_ELF): $(BOOT_STAGE_1_OBJ) 
	$(LD) -T $(BOOT_1_LIKNER) -o $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_BIN): $(BOOT_STAGE_1_ELF)
	$(OBJC) -O binary $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_BIN)
	
# Os Image
$(OS_IMAGE): $(BOOT_STAGE_1_BIN) $(CODE_32_BIN)
	cat $(BOOT_STAGE_1_BIN) $(CODE_32_BIN) > $(OS_IMAGE)

# Rest
.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) 

clean:
	rm -f obj/kernel/*
	rm -f obj/terminal/*
	rm -f obj/exception_stubs/*
	rm -f -r obj/*.o
	rm -f bin/*
	rm -f elf/*
	rm -f lib/*