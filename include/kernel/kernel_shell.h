#pragma once

#include <stdint.h>
#include "terminal_output.h"

namespace kernel
{
    class shell
    {
        static constexpr uint8_t command_capacity{128};
        char command_buffer[command_capacity + 1];
        terminal::output console;
        char* current_data;
        bool command_ready;

        public:
            constexpr shell() noexcept: command_buffer{'\0'}, console{}, current_data(command_buffer), command_ready{false} {}
            void reset() noexcept;
            bool push_character(char) noexcept;
            bool backspace() noexcept;
            void handle_input() noexcept;
            
            [[gnu::always_inline]]
            void submit() noexcept
            {
                *current_data = '\0';
                command_ready = true;
            }
            
            [[gnu::always_inline]]
            inline bool has_command() const noexcept { return command_ready; }

            [[gnu::always_inline]]
            inline bool is_empty() const noexcept { return command_size() == 0; }
            
            // This needs to be a submitted command or else it is invalid
            [[gnu::always_inline]]
            inline const char* command() const noexcept { return command_buffer; }
            
            [[gnu::always_inline]]
            inline uint8_t max_command_size() const noexcept { return command_capacity; }
            
            [[gnu::always_inline]]
            inline uint8_t command_size() const noexcept { return static_cast<uint8_t>(current_data - command_buffer); }

            [[gnu::always_inline]]
            inline terminal::output* get_console() noexcept { return &console; }


            terminal::output& operator<<(char c) noexcept
            {
                return console << c;
            }
    };
}