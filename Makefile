# ----------------Compiling Configuration--------------------
CC = i686-elf-g++

AS = i686-elf-as
LD = i686-elf-ld
OBJC = i686-elf-objcopy
OBJDUMP = i686-elf-objdump -d

MAKE_LIB = i686-elf-ar rcs

QEMU = qemu-system-x86_64

COMPILE_FLAGS = -std=gnu++17 -D_MM_MALLOC_H_INCLUDED -ffreestanding -O3 -Wall -Wextra -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -fno-stack-protector -fcallgraph-info=su -mgeneral-regs-only
LINKING_FLAGS = --gc-sections

SECTOR_SIZE = 512

INCLUDE_MAP_FILE = -Map=output.map

CI_FILES = ci_files
#----------------------- Mk Declaretions ---------------------------------------
include mk_files/tools/decl.mk
include mk_files/apps/decl.mk
include mk_files/assembly/decl.mk
include mk_files/drivers/decl.mk
include mk_files/kernel/decl.mk
include mk_files/link_scripts/decl.mk
include mk_files/stack_calculator/decl.mk
include mk_files/terminal/decl.mk
include mk_files/cpu/decl.mk
include mk_files/io/decl.mk

#------------------------------ Include MK Libraries ---------------------------------
include mk_files/lib/decl.mk

# -----------------------Kenrel Main-------------------------------
MAIN_H = main.h
MAIN_CPP = main.cpp
MAIN_OBJ = obj/main.o

#-------------------------Stack Calculator--------------------------------

# ----------------------Rules--------------------------------

all: $(OS_IMAGE) $(KERNEL_DISASM) $(CI_FILES)

#------------------------  MK Rules ---------------------------------
include mk_files/apps/rules.mk
include mk_files/assembly/rules.mk
include mk_files/drivers/rules.mk
include mk_files/kernel/rules.mk
include mk_files/stack_calculator/rules.mk
include mk_files/terminal/rules.mk
include mk_files/cpu/rules.mk
include mk_files/io/rules.mk

#------------------------------ Include MK Librarys ---------------------------------
include mk_files/lib/rules.mk

#--------------------------------------Kernel Main Rules------------------------------------------------------------
$(MAIN_OBJ): $(MAIN_CPP) $(MAIN_H)
	$(CC) $(COMPILE_FLAGS) -c $(MAIN_CPP) -o $(MAIN_OBJ)

# Rest
.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE)

clean:
	rm -f obj/apps/shell/*
	rm -f obj/boot/*
	rm -f obj/drivers/*
	rm -f obj/exception_stubs/*
	rm -f obj/kernel/*
	rm -f obj/terminal/*
	rm -f obj/io/*
	rm -f obj/*.o
	rm -f bin/*
	rm -f elf/*
	rm -f lib/*.a
	rm -f ci_files/*