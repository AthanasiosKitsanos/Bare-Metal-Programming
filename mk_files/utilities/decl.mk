#-----------------------------Terminal Source Files----------------------------------------
OUTPUT_H = utilities/io/output/terminal_output.h
OUTPUT_CPP = utilities/io/output/terminal_output.cpp
OUTPUT_OBJ = obj/terminal/terminal_output.o

INPUT_H = utilities/io/input/terminal_input.h
INPUT_CPP = utilities/io/input/terminal_input.cpp
INPUT_OBJ = obj/terminal/terminal_input.o

VGA_H = utilities/vga/vga_text_buffer/terminal_vga_text_buffer.h
VGA_CPP = utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp
VGA_OBJ = obj/terminal/terminal_vga_text_buffer.o

CURSOR_H = utilities/vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h
CURSOR_CPP = utilities/vga/vga_hardware_cursor/terminal_vga_hardware_cursor.cpp
CURSOR_OBJ = obj/terminal/terminal_vga_hardware_cursor.o

# Internals
IO_H = utilities/internals/terminal_io_registers.h

#Include Folders
INCLUDE_UTILITIES_FOLDER = -I utilities