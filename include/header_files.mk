include include/drivers/driver_files.mk
include include/kernel/kernel_files.mk
include include/terminal/terminal_files.mk

INCLUDE_ALL_FOLDERS = $(INCLUDE_TERMINAL_FOLDER) $(INCLUDE_KERNEL_ALL_FOLDERS) $(INCLUDE_DRIVERS_ALL_FOLDERS)