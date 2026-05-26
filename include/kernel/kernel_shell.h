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
        char* writable_data;
        char* data_end;
        const char* buffer_end;
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
        inline void move_left() noexcept { --writable_data; }

        [[gnu::always_inline]]
        inline void move_right() noexcept { ++writable_data; }

        public:
            explicit shell(terminal::output* out) noexcept: command_buffer{'\0'}, console{out}, writable_data(command_buffer), data_end{command_buffer}, buffer_end{command_buffer + command_capacity}, length{0}, command_ready{false} {}
            void reset() noexcept;
            bool push_character(char) noexcept;
            bool backspace() noexcept;
            void read_command() noexcept;
            
            [[gnu::always_inline]]
            void submit() noexcept
            {
                while(*data_end == ' ') --data_end;
                *(++data_end) = '\0';
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
            inline uint8_t command_size() const noexcept { return static_cast<uint8_t>(writable_data - command_buffer); }

            // Navigation Controls
            [[gnu::always_inline]]
            inline bool move_arrow_left() noexcept
            {
                if(writable_data == command_buffer) return false;
                move_left();
                return true;
            }

            [[gnu::always_inline]]
            inline bool move_arrow_right() noexcept
            {
                if((writable_data + 1) == data_end) return false;
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