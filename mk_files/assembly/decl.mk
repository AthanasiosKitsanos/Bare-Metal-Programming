# ---------------------Boot stage 1---------------------------
BOOT_STAGE_1 = assembly/boot_stage_1.S
BOOT_STAGE_1_OBJ = obj/boot/boot_stage_1.o

# ---------------------Boot stage 2---------------------------
BOOT_STAGE_2 = assembly/boot_stage_2.S
BOOT_STAGE_2_OBJ = obj/boot/boot_stage_2.o

# ------------------------Pm Entry---------------------------
PM_ENTRY = assembly/pm_entry.S
PM_ENTRY_OBJ = obj/boot/pm_entry.o

#-------------------------Kernel Entry
KENREL_ENTRY = assembly/kernel_entry.S
KERNEL_ENTRY_OBJ = obj/boot/kernel_entry.o

#-------------------------Inc Folder ------------------------
STAGE_2_SECTORS = assembly/internal/stage_2_sectors.inc

#-------------------------Common Interrupt Entry-------------------------------
INTERRUPT_ENTRY_S = assembly/common_interrupt_entry.S
INTERRUPT_ENTRY_OBJ = obj/boot/common_interrupt_entry.o

#----------------------Boot Code 16 -----------------------------
CODE_16_ELF = elf/code_16.elf
CODE_16_BIN = bin/code_16.bin

#---------------------Def Sym-------------------------------
DEF_KERNEL_STACK = --defsym=_kernel_raw_stack
DEF_INTERRUPT_STACK = --defsym=_interrupt_raw_stack

#---------------------Kernel ELF-----------------------------
KERNEL_ELF = elf/kernel.elf
KERNEL_BIN = bin/kernel.bin
KERNEL_DISASM = kernel_dis.txt

# --------------------Code 32--------------------------------

CODE_32_ELF = elf/code_32.elf
CODE_32_BIN = bin/code_32.bin
CODE_32_DISASM = code_32_dis.txt

# ------------------------OS Image---------------------------
OS_IMAGE = bin/os_image.bin