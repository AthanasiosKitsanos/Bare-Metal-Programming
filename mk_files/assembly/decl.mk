# ---------------------Boot stage 1---------------------------
BOOT_STAGE_1 = assembly/boot/boot_stage_1.S
BOOT_STAGE_1_OBJ = obj/boot/boot_stage_1.o

# ---------------------Boot stage 2---------------------------
BOOT_STAGE_2 = assembly/boot/boot_stage_2.S
BOOT_STAGE_2_OBJ = obj/boot/boot_stage_2.o

# ------------------------Pm Entry---------------------------
PM_ENTRY = assembly/boot/pm_entry.S
PM_ENTRY_OBJ = obj/boot/pm_entry.o

#-------------------------Inc Folder ------------------------
STAGE_2_SECTORS = assembly/boot/inc/stage_2_sectors.inc
INCLUDE_INC_FOLDER = -I assembly/boot/inc

#-------------------------Common Interrupt Entry-------------------------------
INTERRUPT_ENTRY_S = assembly/exception_stubs/common_interrupt_entry.S
INTERRUPT_ENTRY_OBJ = obj/exception_stubs/commom_interrupt_entry.o