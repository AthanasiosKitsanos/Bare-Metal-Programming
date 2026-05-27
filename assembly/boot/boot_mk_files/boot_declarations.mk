# ---------------------Boot stage 1---------------------------
BOOT_STAGE_1 = assembly/boot/boot_stage_1.S
BOOT_STAGE_1_OBJ = obj/boot/boot_stage_1.o

# ---------------------Boot stage 2---------------------------
BOOT_STAGE_2_TOTAL_SECTORSz = assembly/boot/inc/boot_stage_2_total_sectors.inc
BOOT_STAGE_2 = assembly/boot/boot_stage_2.S
BOOT_STAGE_2_OBJ = obj/boot/boot_stage_2.o

# ------------------------Pm Entry---------------------------
PM_ENTRY = assembly/boot/pm_entry.S
PM_ENTRY_OBJ = obj/boot/pm_entry.o

#----------------------Boot Code 16 -----------------------------
BOOT_CODE_16_ELF = elf/code_16.elf
BOOT_CODE_16_BIN = bin/code_16.bin
BOOT_CODE_16_LIKNER = links/code_16.ld