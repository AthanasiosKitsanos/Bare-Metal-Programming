#-----------------Drivers Internal-----------------------
KEYBOARD_KEY_LIST_H = include/drivers/internal/kernel_keyboard_key_list.h

#-------------------Keyboard Headers----------------------------------
KEYBOARD_H = include/drivers/keyboard.h

#--------------------Include Folders------------------------
INCLUDE_DRIVERS_FOLDER = -Iinclude/drivers
INCLUDE_DRIVERS_INTERNAL_FOLDER = -Iinclude/drivers/internal
INCLUDE_DRIVERS_ALL_FOLDERS = $(INCLUDE_DRIVERS_FOLDER) $(INCLUDE_DRIVERS_INTERNAL_FOLDER)