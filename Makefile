# ----------------Compiling Configuration--------------------
CC = i686-elf-g++

AS = i686-elf-as
LD = i686-elf-ld
OBJC = i686-elf-objcopy

QEMU = qemu-system-x86_64

COMPILE_FLAGS = -std=gnu++17 -ffreestanding -O3 -Wall -Wextra -fno-exceptions -fno-rtti
LINKING_FLAGS = -ffreestanding -O3 -nostdlib

# File Sources

# ---------------------Boot stage 1---------------------------
BOOT_STAGE_1 = boot/boot_stage_1.S
BOOT_STAGE_1_OBJ = obj/boot_stage_1.o
BOOT_STAGE_1_ELF = elf/boot_stage_1.elf
BOOT_STAGE_1_BIN = bin/boot_stage_1.bin
BOOT_1_LIKNER = links/boot_1_linker.ld

# ---------------------Boot stage 2---------------------------
BOOT_STAGE_2 = boot/boot_stage_2.S
BOOT_STAGE_2_OBJ = obj/boot_stage_2.o
BOOT_STAGE_2_ELF = elf/boot_stage_2.elf
BOOT_STAGE_2_BIN = bin/boot_stage_2.bin
BOOT_2_LINKER = links/boot_2_linker.ld

# -----------------------Kenrel-------------------------------
KERNEL_CPP = kernel/kernel.cpp
KERNEL_OBJ = obj/kernel.o
KERNEL_ELF = elf/kernel.elf
KERNEL_LINKER = links/kernel_linker.ld
KERNEL_BIN = bin/kernel.bin

TERMINAL_H = include/terminal.h
VGA_H = include/vga_text_buffer.h

TERMINAL_CPP = src/terminal.cpp
TERMINAL_OBJ = obj/terminal.o
VGA_CPP = src/vga_text_buffer.cpp
VGA_OBJ = obj/vga_text_buffer.o

INCLUDE = include

# ------------------------Pm Entry---------------------------
PM_ENTRY = boot/pm_entry.S
PM_ENTRY_OBJ = obj/pm_entry.o

# ------------------------OS Image---------------------------
OS_IMAGE = bin/os_image.bin

# ----------------------Rules--------------------------------

all: $(OS_IMAGE)

# Boot 1
$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1)
	$(AS) $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_ELF): $(BOOT_STAGE_1_OBJ)
	$(LD) -T $(BOOT_1_LIKNER) -o $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_BIN): $(BOOT_STAGE_1_ELF)
	$(OBJC) -O binary $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_BIN)

# Kernel
$(KERNEL_OBJ): $(KERNEL_CPP) $(TERMINAL_H) $(VGA_H)
	$(CC) $(COMPILE_FLAGS) -I$(INCLUDE) -c $(KERNEL_CPP) -o $(KERNEL_OBJ)

$(VGA_OBJ): $(VGA_CPP) $(VGA_H)
	$(CC) $(COMPILE_FLAGS) -I$(INCLUDE) -c $(VGA_CPP) -o $(VGA_OBJ)

$(TERMINAL_OBJ): $(VGA_H) $(TERMINAL_H)
	$(CC) $(COMPILE_FLAGS) -I$(INCLUDE) -c $(TERMINAL_CPP) -o $(TERMINAL_OBJ)

# PM Entry
$(PM_ENTRY_OBJ): $(PM_ENTRY)
	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# Boot 2
$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

$(BOOT_STAGE_2_ELF): $(BOOT_STAGE_2_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(TERMINAL_OBJ)
	$(LD) -T $(BOOT_2_LINKER) -o $(BOOT_STAGE_2_ELF) \
		$(BOOT_STAGE_2_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_OBJ) $(VGA_OBJ) $(TERMINAL_OBJ)

$(BOOT_STAGE_2_BIN): $(BOOT_STAGE_2_ELF)
	$(OBJC) -O binary $(BOOT_STAGE_2_ELF) $(BOOT_STAGE_2_BIN)
	size=$$(wc -c < $(BOOT_STAGE_2_BIN)); \
	if [ $$size -gt 2048 ]; then \
		echo "Stage 2 is too large: $$size bytes (max 2048)."; \
		exit 1; \
	fi
	truncate -s 2048 $(BOOT_STAGE_2_BIN)

# Os Image
$(OS_IMAGE): $(BOOT_STAGE_1_BIN) $(BOOT_STAGE_2_BIN)
	cat $(BOOT_STAGE_1_BIN) $(BOOT_STAGE_2_BIN) > $(OS_IMAGE)
	rm -f $(BOOT_STAGE_1_BIN)
	rm -f $(BOOT_STAGE_2_BIN)
	rm -f $(BOOT_STAGE_1_ELF)
	rm -f $(BOOT_STAGE_2_ELF)

# Rest
.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) 

clean:
	rm -f obj/*
	rm -f bin/*