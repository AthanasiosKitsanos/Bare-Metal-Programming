#pragma once

#include <stddef.h>
#include <stdint.h>
#include "tools/fixed_string.h"

namespace terminal::format_parser
{
    enum class output_type: uint8_t
    {
        plain_text = 0,
        argument = 1
    };

    enum class representation: uint8_t
    {
        dec = 0,
        hex = 1,
        bin = 2,
        lex = 3
    };

    struct token
    {
        size_t base{0};
        size_t length{0};
        output_type o_type{output_type::plain_text};
        representation rep{representation::dec};
    };
    static_assert(sizeof(token) == 12);

    enum class lexeme_kind: uint8_t
    {
        simple_character = 0,
        argument_placeholder = 1,
        escape = 2,
        wrong_syntax = 3
    };

    struct report
    {
        uint8_t width{1};
        lexeme_kind kind{lexeme_kind::simple_character};
        representation rep{representation::dec};
    };

    // Recognizes the lexeme that starts at 'index'. Does not loop.
    //
    // Bounds safety relies on two rules working together:
    //   1. Caller contract: index <= N - 2 (only real characters, never the '\0').
    //      Therefore index + 1 <= N - 1, so reading data[index + 1] is always in bounds.
    //   2. Sentinel: data[N - 1] is always '\0'. data[index + 2] is read only when
    //      data[index + 1] is a specifier letter ('h', 'b', 'l'). A letter is not '\0',
    //      so index + 1 <= N - 2, which means index + 2 <= N - 1, still in bounds.

    // Changing the caller's loop bound or the order of the reads breaks this proof.
    template<size_t N>
    consteval report write_report(const tools::fixed_string<N> Fmt, const size_t index) noexcept
    {
        if(Fmt.data[index] != '{' && Fmt.data[index] != '}') return report{};
        if(Fmt.data[index] == '{')
        {
            switch(Fmt.data[index + 1])
            {
                case '{':
                    return report{2, lexeme_kind::escape};
                case '}':
                    return report{2, lexeme_kind::argument_placeholder};
                case 'h':
                    if(Fmt.data[index + 2] == '}') return report{3, lexeme_kind::argument_placeholder, representation::hex};
                    break;
                case 'b':
                    if(Fmt.data[index + 2] == '}') return report{3, lexeme_kind::argument_placeholder, representation::bin};
                    break;
                case 'l':
                    if(Fmt.data[index + 2] == '}') return report{3, lexeme_kind::argument_placeholder, representation::lex};
                    break;
                default:
                    break;
            }
            return report{0, lexeme_kind::wrong_syntax};
        }
        if(Fmt.data[index] == '}' && Fmt.data[index + 1] == '}') return report{2, lexeme_kind::escape};
        return report{0, lexeme_kind::wrong_syntax};
    }

    template<size_t Capacity>
    struct token_collection
    {
        static_assert(Capacity > 0, "The token collection needs at least 1 element");
        token entries[Capacity];
        size_t counter{0};

        [[gnu::always_inline]]
        token& operator[](const size_t index) noexcept
        {
            return entries[index];
        }
    };

    template<size_t N>
    consteval token_collection<N - 1> generate_token_collection(const tools::fixed_string<N> Fmt) noexcept
    {
        static_assert(N >= 2, "String must not be empty");
        const size_t end{N - 1};
        token_collection<end> collection{};
        size_t text_start{0};
        for(size_t i{0}; i < end;)
        {
            report rpt{write_report(Fmt, i)};
            output_type type{};
            switch(rpt.kind)
            {
                case lexeme_kind::argument_placeholder:
                    text_start = i + rpt.width;
                    break;
                case lexeme_kind::escape:
                    text_start = i + 1;
                    break;
                default:
                    break;
            }
            collection[collection.counter++ ] = token{text_start, end - i + rpt.width, ,rpt.rep};
            i += rpt.width;
        }
        return collection;
    }
}
