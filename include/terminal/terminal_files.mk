#---------------------------Terminal Header Files-------------------------------------------
IO_H = include/terminal/terminal_io_registers.h

OUTPUT_H = include/terminal/terminal_output.h

INPUT_H = include/terminal/terminal_input.h

VGA_H = include/terminal/terminal_vga_text_buffer.h

CURSOR_H = include/terminal/terminal_vga_hardware_cursor.h

#-----------------------------Terminal Include Files-----------------------------------------
INCLUDE_TERMINAL_INTERNAL_FOLDER = = -Iinclude/terminal/internal
INCLUDE_TERMINAL_FOLDER = -Iinclude/terminal