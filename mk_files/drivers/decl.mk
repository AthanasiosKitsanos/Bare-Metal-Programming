#-----------------Drivers Internal-----------------------
KEYBOARD_KEY_LIST_H = include/drivers/internal/keyboard_key_list_n_map.h

#-------------------Keyboard Headers----------------------------------
KEYBOARD_H = include/drivers/keyboard/keyboard.h
KEYBOARD_CPP = src/drivers/keyboard.cpp
KEYBOARD_OBJ = obj/drivers/keyboard.o

#--------------------Include Folders------------------------
INCLUDE_DRIVERS_FOLDER = -Idrivers
INCLUDE_DRIVERS_INTERNAL_FOLDER = -Idrivers/internal
INCLUDE_DRIVERS_ALL_FOLDERS = $(INCLUDE_DRIVERS_FOLDER) $(INCLUDE_DRIVERS_INTERNAL_FOLDER)