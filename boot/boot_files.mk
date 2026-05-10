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

# ------------------------Pm Entry---------------------------
PM_ENTRY = boot/pm_entry.S
PM_ENTRY_OBJ = obj/pm_entry.o