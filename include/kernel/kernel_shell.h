#pragma once

#include <stdint.h>

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
        bool command_ready;

        [[gnu::always_inline]]
        inline uint8_t control_key_index(const uint8_t key_code) noexcept
        {
            constexpr uint8_t index_mask{0x03};
            return ((key_code - 1) & index_mask);
        }

        friend void escape_handler(kernel::shell* s) noexcept;
        friend void backspace_handler(kernel::shell* s) noexcept;
        friend void tab_handler(kernel::shell* s) noexcept;
        friend void enter_handler(kernel::shell* s) noexcept;

        public:
            explicit shell(terminal::output* out) noexcept: command_buffer{'\0'}, console{out}, current_data(command_buffer), command_ready{false} {}
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
            inline const char* command() const noexcept { return command_buffer; }
            
            [[gnu::always_inline]]
            inline uint8_t max_command_size() const noexcept { return command_capacity; }
            
            [[gnu::always_inline]]
            inline uint8_t command_size() const noexcept { return static_cast<uint8_t>(current_data - command_buffer); }

            [[gnu::always_inline]]
            inline terminal::output* get_console() const noexcept { return console; }
    };

    void escape_handler(kernel::shell* s) noexcept;
    void backspace_handler(kernel::shell* s) noexcept;
    void tab_handler(kernel::shell* s) noexcept;
    void enter_handler(kernel::shell* s) noexcept;
}