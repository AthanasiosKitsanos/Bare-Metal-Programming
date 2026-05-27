#pragma once

#include <stdint.h>
#include "terminal_navigation_table.h"
#include "terminal_control_input_table.h"

namespace terminal
{
    class input
    {
        static constexpr uint8_t command_capacity{128};
        char command_buffer[command_capacity + 1];
        char* writable_data;
        char* data_end;
        bool command_ready;

        // Friend Control Functions
        #define X(name, index)  \
            friend void name##_handler(input*) noexcept;
        CONTROL_KEY_MAPPING
        #undef X

        [[gnu::always_inline]]
        inline void move_left() noexcept { --writable_data; }

        [[gnu::always_inline]]
        inline void move_right() noexcept { ++writable_data; }
        
        public:
            explicit input() noexcept: command_buffer{'\0'}, writable_data(command_buffer), data_end{command_buffer}, command_ready{false} {}
            void reset() noexcept;
            bool push_character(char) noexcept;
            bool backspace() noexcept;
            void read_command() noexcept;
            
            [[gnu::always_inline]]
            void submit() noexcept
            {
                while(data_end > command_buffer && *(data_end - 1) == ' ') --data_end;
                for(char pre_ch{*(data_end - 1)}; data_end > command_buffer && pre_ch == ' '; pre_ch = *(data_end - 1))
                {
                    --data_end;
                }
                *data_end = '\0';
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
                if(writable_data == data_end) return false;
                move_right();
                return true;
            }
    };

    #define X(name, index)  \
        void name##_handler(input*) noexcept;
    CONTROL_KEY_MAPPING
    #undef X

    #define X(name, index)  \
        void name##_handler(input*) noexcept;
    NAVIGATION_KEY_TABLE
    #undef X
}