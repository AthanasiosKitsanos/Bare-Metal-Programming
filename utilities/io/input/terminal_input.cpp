#include "terminal_input.h"
#include "io/output/terminal_output.h"
#include "internals/navigation_handlers.h"
#include "internals/control_input_handlers.h"
#include "keyboard/keyboard.h"

namespace
{
    [[gnu::always_inline]]
    inline void move_data_right(char* end, const char* begin, const uint8_t step) noexcept
    {
        for(; end > begin; --end) *end = *(end - step);
    }

    [[gnu::always_inline]]
    inline void move_data_left(char* begin, const char* end, const uint8_t step) noexcept
    {
        for(; begin < end; ++begin) *begin = *(begin + step);
    }

    [[gnu::always_inline]]
    inline uint8_t string_length(const char* string) noexcept
    {
        uint8_t length{0};
        for(; *string != '\0'; ++string) ++length;
        return length;
    }

    [[gnu::always_inline]]
    inline uint8_t get_control_map_index(const driver::keyboard::keyboard_key key) noexcept
    {
        constexpr uint8_t index_mask{0x03};
        return static_cast<uint8_t>((static_cast<uint16_t>(key) - 1) & index_mask);
    }

}

namespace terminal
{
    input::input(output* out) noexcept: m_output{out}, cursor{input_buffer}, data_end{input_buffer}, input_buffer{}, input_ready{false} 
    {}

    bool input::add_character(const char c) noexcept
    {
        if(buffer_full()) return false;
        if(*cursor != '\0')
        {
            move_data_right(data_end, cursor, 1);
        }
        *cursor = c;
        ++cursor;
        *(++data_end) = '\0';
        return true;
    }

    bool input::add_string(const char* string) noexcept
    {
        const uint8_t length{string_length(string)};
        if(data_end + length >= input_buffer + input_capacity) return false;
        data_end += length;
        if(*cursor != '\0')
        {
            move_data_right(data_end, cursor + length - 1, length);   
        }
        for(; *string != '\0'; ++string)
        {
            *cursor = *string;
            ++cursor;
        }
        return true;
    }

    bool input::delete_character() noexcept
    {
        if(buffer_begin()) return false;
        --cursor;
        move_data_left(cursor, data_end, 1);
        *(--data_end) = '\0';
        return true;
    }

    void input::reset_buffer() noexcept
    {
        cursor = input_buffer;
        data_end = input_buffer;
        *cursor = '\0';
        input_ready = false;
    }

    void input::trim_end() noexcept
    {
        if(data_end == input_buffer) return;
        while(*(data_end - 1) == ' ') --data_end;
        *data_end = '\0';
    }

    void input::start_data_recieving() noexcept
    {
        driver::keyboard::keyboard_event event{};
        char c{'\0'};
        while(!input_ready)
        {
            while(!driver::keyboard::poll_keyboard_event(&event))
            {
                asm volatile("hlt");
            }
            if(driver::keyboard::try_translate_text_event(&event, &c))
            {
                if(add_character(c))
                {
                    *m_output << c;
                    if(cursor != data_end)
                    {
                        m_output->print_string_no_sync(cursor);
                        m_output->move_cursor_to_left_pos(data_end - cursor);
                        m_output->call_cursor_sync();
                    }
                }
            }
            else if(driver::keyboard::is_control_input_candidate_event(&event))
            {
                control_key_dispatch(event.key);
            }
            else if(driver::keyboard::is_navigation_input_candidate_event(&event))
            {
                navigation_key_dispatch(event.key);
            }
        }
    }

    void input::handle_escape() noexcept
    {
        m_output->move_cursor_to_right_pos(data_end - cursor);
        for(uint8_t steps{count()}; steps > 0; --steps)
        {
            m_output->delete_last_char_no_sync();
        }
        m_output->call_cursor_sync();
        reset_buffer();
    }
    
    void input::handle_backspace() noexcept
    {
        if(delete_character())
        {
            m_output->delete_last_char_no_sync();
            if(cursor != data_end)
            {
                m_output->print_string_no_sync(cursor);
                m_output->move_to_next_no_sync();
                m_output->delete_last_char_no_sync();
                m_output->move_cursor_to_left_pos(data_end - cursor);
            }
            m_output->call_cursor_sync();
        }
    }
    
    void input::handle_tab() noexcept
    {
        constexpr const char* spaces{"    "};
        if(add_string(spaces))
        {
            *m_output << spaces;
            if(cursor != data_end)
            {
                m_output->print_string_no_sync(cursor);
                m_output->move_cursor_to_left_pos(data_end - cursor);
                m_output->call_cursor_sync();
            }
        }
    }
    
    void input::handle_enter() noexcept
    {
        if(count() > 0)
        {
            trim_end();
        }
        *m_output << '\n';
        input_ready = true;
    }
    
    void input::control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept
    {
        switch(key)
        {
            #define X(key)  \
                case driver::keyboard::keyboard_key::key:   \
                    handle_##key(); \
                return;
            CONTROL_INPUT_HANDLERS
            #undef X

            default:
                return;
        }
    }

    void input::handle_home() noexcept
    {
        m_output->move_cursor_to_left_pos(cursor - input_buffer);
        cursor = input_buffer;
        m_output->call_cursor_sync();
    }

    void input::handle_arrow_up() noexcept
    {
        return;
    }

    void input::handle_page_up() noexcept
    {
        return;
    }

    void input::handle_arrow_left() noexcept
    {
        if(move_cursor_left()) m_output->move_to_previous();
    }

    void input::handle_arrow_right() noexcept
    {
        if(move_cursor_right()) m_output->move_to_next();
    }

    void input::handle_end() noexcept
    {
        m_output->move_cursor_to_right_pos(data_end - cursor);
        cursor = data_end;
        m_output->call_cursor_sync();
    }

    void input::handle_arrow_down() noexcept
    {
        return;
    }

    void input::handle_page_down() noexcept
    {
        return;
    }

    void input::navigation_key_dispatch(const driver::keyboard::keyboard_key key) noexcept
    {
        switch(key)
        {
            #define X(key)  \
                case driver::keyboard::keyboard_key::key:   \
                    handle_##key(); \
                    return;
            NAVIGATION_HANDLERS
            #undef X

            default:
                return;
        }
    }
}