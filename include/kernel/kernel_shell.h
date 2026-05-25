#pragma once

#include <stdint.h>
#include "kernel_navigation_table.h"
#include "kernel_control_input_table.h"

namespace terminal
{
    class output;
}
namespace kernel
{
    class shell
    {
        static constexpr uint8_t command_capacity{128};
        char command_buffer[command_capacity + 1];
        terminal::output* const console;
        char* current_data;
        const char* end;
        uint8_t length;
        bool command_ready;

        // Friend Control Functions
        #define X(name, index)  \
            friend void name##_handler(shell*) noexcept;
        CONTROL_KEY_MAPPING
        #undef X

        // Friend Navigation Functions
        #define X(name, index)  \
            friend void name##_handler(shell*) noexcept;
        NAVIGATION_KEY_TABLE
        #undef X

        [[gnu::always_inline]]
        inline void move_left() noexcept { --current_data; }

        [[gnu::always_inline]]
        inline void move_right() noexcept { ++current_data; }

        public:
            explicit shell(terminal::output* out) noexcept: command_buffer{'\0'}, console{out}, current_data(command_buffer), end{command_buffer + command_capacity}, length{0}, command_ready{false} {}
            void reset() noexcept;
            bool push_character(char) noexcept;
            bool backspace() noexcept;
            void read_command() noexcept;
            
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
            inline uint8_t max_command_size() const noexcept { return command_capacity; }
            
            [[gnu::always_inline]]
            inline uint8_t command_size() const noexcept { return static_cast<uint8_t>(current_data - command_buffer); }

            // Navigation Controls
            [[gnu::always_inline]]
            inline bool move_arrow_left() noexcept
            {
                if(current_data == command_buffer) return false;
                move_left();
                return true;
            }

            [[gnu::always_inline]]
            inline bool move_arrow_right() noexcept
            {
                if(*(current_data + 1) == '\0' ) return false;
                move_right();
                return true;
            }
    };

    #define X(name, index)  \
        void name##_handler(kernel::shell*) noexcept;
    CONTROL_KEY_MAPPING
    #undef X

    #define X(name, index)  \
        void name##_handler(kernel::shell*) noexcept;
    NAVIGATION_KEY_TABLE
    #undef X
}