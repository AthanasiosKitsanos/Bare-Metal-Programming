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
include mk_files/apps/decl.mk
include mk_files/assembly/decl.mk
include mk_files/drivers/decl.mk
include mk_files/kernel/decl.mk
include mk_files/links/decl.mk
include mk_files/utilities/decl.mk

#------------------------------ Include MK Librarys ---------------------------------
include mk_files/lib/decl.mk

# -----------------------Kenrel Main-------------------------------
MAIN_CPP = main.cpp
MAIN_OBJ = obj/main.o

# ----------------------Rules--------------------------------

all: $(OS_IMAGE)

#------------------------ Source MK Files ---------------------------------
include mk_files/apps/rules.mk
include mk_files/assembly/rules.mk
include mk_files/drivers/rules.mk
include mk_files/kernel/rules.mk
include mk_files/utilities/rules.mk

#------------------------------ Include MK Librarys ---------------------------------
include mk_files/lib/rules.mk

#--------------------------------------Kernel Main Rules------------------------------------------------------------
$(MAIN_OBJ): $(MAIN_CPP)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_DRIVERS_FOLDER) $(INCLUDE_KERNEL_FOLDER) $(INCLUDE_UTILITIES_FOLDER) -c $(MAIN_CPP) -o $(MAIN_OBJ)

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