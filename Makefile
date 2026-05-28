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

#-----------------------Include Mk Files---------------------------------------
include apps/app_declarations.mk
include assembly/assembly_declarations.mk
include include/header_files.mk
include src/src_declarations.mk
include utilities/utilities_files.mk

#------------------------------ Include MK Librarys ---------------------------------
include lib/lib_mk_files/lib_declarations.mk

# -----------------------Kenrel Main-------------------------------
MAIN_CPP = main.cpp
MAIN_OBJ = main.o


# #-----------------------Include All Folders----------------------------------------
# INCLUDE_ALL_FOLDERS = $(INCLUDE_TERMINAL_FOLDER) $(INCLUDE_KERNEL_ALL_FOLDERS) $(INCLUDE_DRIVERS_ALL_FOLDERS)

# # ------------------------Library----------------------------
# # Kernel Library
# KERNEL_LIB_FILES = $(KERNEL_OBJ) $(VGA_OBJ) $(TERMINAL_OUTPUT_OBJ) $(CURSOR_OBJ) $(LOGGER_OBJ) $(ASSERT_OBJ) $(IDT_ENTRY_OBJ) \
# 	$(EXCEPTIONS_OBJ) $(PIC_OBJ) $(TIMER_OBJ) $(PIT_OBJ) $(KEYBOARD_OBJ) $(KERNEL_SHELL_OBJ)
# KERNEL_A = lib/libkernel.a
# LINK_KERNEL_LIB = -Llib -lkernel

# # App Libraries
# APP_LIB_FILES = $(SHELL_OBJ)
# APPS_A = lib/libapps.a
# LINK_APPS = -Llib -lapps

# LINK_ALL_LIBS = $(LINK_APPS) $(LINK_KERNEL_LIB)
# # --------------------Code 32--------------------------------
# CODE_32_ELF = elf/code_32.elf
# CODE_32_BIN = bin/code_32.bin
# CODE_32_LINKER = links/code_32.ld

# # ------------------------OS Image---------------------------
# OS_IMAGE = bin/os_image.bin


# ----------------------Rules--------------------------------

all: $(OS_IMAGE)

#------------------------ Source MK Files ---------------------------------
include apps/app_rules.mk
include assembly/assembly_rules.mk
include src/src_rules.mk

#------------------------------ Include MK Librarys ---------------------------------
include lib/lib_mk_files/lib_rules.mk

#--------------------------------------Kernel Main Rules------------------------------------------------------------
$(MAIN_OBJ): $(MAIN_CPP)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(MAIN_CPP) -o $(MAIN_OBJ)


# #--------------------------------------Terminal Rules----------------------------------------------------------
# # Terminal
# $(TERMINAL_OUTPUT_OBJ): $(VGA_H) $(TERMINAL_H) $(TERMINAL_OUTPUT_CPP) $(IO_H) $(LOGGER_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(TERMINAL_OUTPUT_CPP) -o $(TERMINAL_OUTPUT_OBJ)

# # VGA cursor
# $(CURSOR_OBJ): $(IO_H) $(CURSOR_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_TERMINAL_FOLDER) -c $(CURSOR_CPP) -o $(CURSOR_OBJ)

# # VGA Buffer
# $(VGA_OBJ): $(VGA_CPP) $(VGA_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_TERMINAL_FOLDER) -c $(VGA_CPP) -o $(VGA_OBJ)


# # Kernel Logger
# $(LOGGER_OBJ): $(LOGGER_H) $(LOGGER_CPP) $(TERMINAL_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(LOGGER_CPP) -o $(LOGGER_OBJ)

# # Kernel Assert
# $(ASSERT_OBJ): $(ASSERT_CPP) $(ASSERT_H) $(LOGGER_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(ASSERT_CPP) -o $(ASSERT_OBJ)

# # Kenrel IDT
# $(IDT_ENTRY_OBJ): $(IDT_ENTRY_CPP) $(IDT_ENTRY_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_KERNEL_FOLDER) -c $(IDT_ENTRY_CPP) -o $(IDT_ENTRY_OBJ)

# # Kernel Timer
# $(TIMER_OBJ): $(TIMER_CPP) $(TIMER_H) $(LOGGER_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(TIMER_CPP) -o $(TIMER_OBJ)

# # Kernel Exceptions
# $(EXCEPTIONS_OBJ): $(EXCEPTIONS_CPP) $(EXCEPTIONS_H) $(LOGGER_H) $(IDT_ENTRY_H) $(INTERRUPT_FRAME_H) $(PIC_H) $(TIMER_H) $(PIT_H) $(KEYBOARD_H) $(KERNEL_CPU_INTERRUPTS_H) $(KERNEL_HARDWARE_INTERRUPRS_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) $(INCLUDE_KERNEL_INTERNAL_FOLDER) -c $(EXCEPTIONS_CPP) -o $(EXCEPTIONS_OBJ)

# # Kernel PIC
# $(PIC_OBJ): $(PIC_CPP) $(PIC_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(PIC_CPP) -o $(PIC_OBJ)

# # Kernel PIT
# $(PIT_OBJ): $(PIT_CPP) $(PIT_H) $(IO_H) $(LOGGER_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) -c $(PIT_CPP) -o $(PIT_OBJ)

# #-------------------------Drivers----------------------------------------------------------------------------------
# # Keyboard
# $(KEYBOARD_OBJ): $(KEYBOARD_CPP) $(KEYBOARD_H) $(IO_H) $(KEYBOARD_KEY_LIST_H) $(INTERRUPT_GUARD_H)
# 	$(CC) $(COMPILE_FLAGS) $(INCLUDE_ALL_FOLDERS) $(INCLUDE_DRIVERS_INTERNAL_FOLDER) -c $(KEYBOARD_CPP) -o $(KEYBOARD_OBJ)

# #---------------------------------Kernel Library Rules-------------------------------------------------------------------------
# # KERNEL_A
# $(KERNEL_A): $(KERNEL_LIB_FILES)
# 	$(MAKE_LIB) $(KERNEL_A) $(KERNEL_LIB_FILES)

# # APP_A
# $(APP_A): $(APP_LIB_FILES)
# 	$(MAKE_LIB) $(APP_A) $(APP_LIB_FILES)

# #----------------------------------Assembly Rules----------------------------------------------------------------------------
# # PM Entry
# $(PM_ENTRY_OBJ): $(PM_ENTRY)
# 	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# # Common Interrupt Entry
# $(INTERRUPT_ENTRY_OBJ): $(INTERRUPT_ENTRY_S) 
# 	$(AS) $(INTERRUPT_ENTRY_S) -o $(INTERRUPT_ENTRY_OBJ)

# # Boot 2
# $(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
# 	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

# # Code 32
# $(CODE_32_ELF): $(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_A)
# 	$(LD) -T $(CODE_32_LINKER) -o $(CODE_32_ELF) \
# 		$(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(LINK_ALL_LIBS)

# $(CODE_32_BIN): $(CODE_32_ELF)
# 	$(OBJC) -O binary $(CODE_32_ELF) $(CODE_32_BIN)
# 	size=$$(wc -c < $(CODE_32_BIN) ); \
# 	sectors=$$(( ($$size + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
# 	padded_size=$$(( $$sectors * $(SECTOR_SIZE) )); \
# 	if [ $$sectors -gt 127 ]; then \
# 		echo "Stage 2 is too large: $$sectors sectors (BIOS read limit exceeded)."; \
# 		exit 1; \
# 	fi; \
# 	truncate -s $$padded_size $(CODE_32_BIN)

# # Loading Configuration
# $(BOOT_STAGE_2_INC): $(CODE_32_BIN)
# 	size=$$(wc -c < $(CODE_32_BIN)); \
# 	sectors=$$(( $$size / $(SECTOR_SIZE) )); \
# 	if [ -z "$$sectors" ]; then \
# 		echo "Failed to compute stage 2 sector count."; \
# 		exit 1; \
# 	fi; \
# 	printf '.set BOOT_STAGE_2_SECTORS, %s\n' "$$sectors" > $(BOOT_STAGE_2_INC)

# # Boot 1
# $(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1) $(BOOT_STAGE_2_INC)
# 	$(AS) -I inc $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

# $(BOOT_STAGE_1_ELF): $(BOOT_STAGE_1_OBJ) 
# 	$(LD) -T $(BOOT_1_LIKNER) -o $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_OBJ)

# $(BOOT_STAGE_1_BIN): $(BOOT_STAGE_1_ELF)
# 	$(OBJC) -O binary $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_BIN)
	
# # Os Image
# $(OS_IMAGE): $(BOOT_STAGE_1_BIN) $(CODE_32_BIN)
# 	cat $(BOOT_STAGE_1_BIN) $(CODE_32_BIN) > $(OS_IMAGE)

# Rest
.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) 

clean:
	rm -f obj/apps/shell/*
	rm -f obj/boot/*
	rm -f obj/drivers/*
	rm -f obj/exception_stubs/*
	rm -f obj/hardware_exceptions/*
	rm -f obj/kernel/*
	rm -f obj/terminal/*
	rm -f bin/*
	rm -f elf/*
	rm -f lib/*.a