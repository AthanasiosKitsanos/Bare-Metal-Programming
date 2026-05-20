#include "kernel_shell.h"

namespace kernel
{
    void shell::reset() noexcept
    {
        current_data = command_buffer;
        *current_data = '\0';
        command_ready = false;
    }

    bool shell::push_character(char c) noexcept
    {
        if(command_ready || current_data >= (command_buffer + command_capacity)) return false;
        *current_data = c;
        ++current_data;
        *current_data = '\0';
        return true;
    }

    bool shell::backspace() noexcept
    {
        if(command_ready || command_size() == 0) return false;
        --current_data;
        *current_data = '\0';
        return true; 
    }
}