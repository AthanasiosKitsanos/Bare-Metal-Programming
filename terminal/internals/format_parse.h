#pragma once

#include <stddef.h>
#include <stdint.h>
#include "tools/fixed_string.h"
#include "tools/sequence.h"

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
        bin = 1,
        hex = 2,
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
        wrong_syntax = 0, // Default value, never returns
        argument_placeholder = 1, // For valid argument
        escape = 2, // escape character "{{" or "}}", print '{' or '}', i.e. "{{hello}}" -> "{hello}"
        unclosed_brace = 3, // "{h" for when having a representation format
        unclosed_or_opening_escape = 4, // "{"
        no_opening_or_escape_closed_brace = 5, // "}"
        contiguous_reprs = 6, // "{hx}" or "{hl}"
        invalid_repr = 7, // "{x}" anything that is not h,b or l representation formats
    };

    struct report
    {
        uint8_t width{0};
        lexeme_kind kind{lexeme_kind::wrong_syntax};
        representation rep{representation::dec};
    };

    void format_error_internal_parser_bug_report_this();
    void format_error_unclosed_brace_add_closing_brace();
    void format_error_unclosed_brace_add_closing_or_escape_brace();
    void format_error_no_opening_brace_add_opening_or_escape_brace();
    void format_error_contiguous_reprs_add_only_one();
    void format_error_invalid_repr_did_you_mean_h_or_b_or_l();

    // Recognizes the lexeme that starts at 'index'. Does not loop.
    //
    // Bounds safety relies on two rules working together:
    //   1. Caller contract: index <= N - 2 (only real characters, never the '\0').
    //      Therefore index + 1 <= N - 1, so reading data[index + 1] is always in bounds.
    //   2. Sentinel: data[N - 1] is always '\0'. data[index + 2] is read only when
    //      data[index + 1] is not '\0'. A letter is not '\0',
    //      so index + 1 <= N - 2, which means index + 2 <= N - 1, still in bounds.

    // Changing the caller's loop bound or the order of the reads breaks this proof.
    template<size_t N>
    consteval report locate_characteristics(const tools::fixed_string<N> Fmt, const size_t index) noexcept
    {
        if(Fmt.data[index] == '{')
        {
            switch(Fmt.data[index + 1])
            {
                case '{':
                    return report{2, lexeme_kind::escape};
                case '}':
                    return report{2, lexeme_kind::argument_placeholder};
                case '\0':
                    return report{0, lexeme_kind::unclosed_or_opening_escape};
                default:
                    {
                        char c{Fmt.data[index + 1]};
                        if(c != 'b' && c != 'h' && c != 'l') return report{0, lexeme_kind::invalid_repr};
                        uint8_t rep{static_cast<uint8_t>((c == 'b') * static_cast<uint8_t>(representation::bin))};
                        rep |= static_cast<uint8_t>((c == 'h') * static_cast<uint8_t>(representation::hex));
                        rep |= static_cast<uint8_t>((c == 'l') * static_cast<uint8_t>(representation::lex));
                        c = Fmt.data[index + 2];
                        if(c == '}') return report{3, lexeme_kind::argument_placeholder, static_cast<representation>(rep)};
                        if(c == '\0') return report{0, lexeme_kind::unclosed_brace};
                        return report{0, lexeme_kind::contiguous_reprs};
                    }
            }
        }
        if(Fmt.data[index] == '}')
        {
            if(Fmt.data[index + 1] != '}') return report{0, lexeme_kind::no_opening_or_escape_closed_brace};
            return report{2, lexeme_kind::escape};
        }
        return report{};
    }

    template<size_t Capacity>
    struct token_collection
    {
        static_assert(Capacity > 0, "token_collection: collection must have at least 1 element");
        token entries[Capacity];
        size_t counter{0};

        [[gnu::always_inline]]
        constexpr token& operator[](const size_t index) noexcept { return entries[index]; }

        [[gnu::always_inline]]
        constexpr const token& operator[](const size_t index) const noexcept { return entries[index]; }
    };

    void wrong_syntax_error() noexcept;

    template<size_t N>
    consteval token_collection<N - 1> generate_token_collection(const tools::fixed_string<N> Fmt) noexcept
    {
        static_assert(N >= 2, "generate_token_collection: String must not be empty");
        // variable end points to the termination character '\0'
        constexpr size_t end{N - 1};
        token_collection<end> collection{};
        size_t text_start{0};
        const auto close_text = [&](const size_t stop)
        {
                if(stop > text_start)
                {
                    collection[collection.counter++] = token{text_start, (stop - text_start), output_type::plain_text};
                }
        };

        for(size_t i{0}; i < end;)
        {
            const char c{Fmt.data[i]};
            if(c != '{' && c != '}')
            {
                ++i;
                continue;
            }
            close_text(i);
            report rpt{locate_characteristics(Fmt, i)};
            switch(rpt.kind)
            {
                case lexeme_kind::argument_placeholder:
                    collection[collection.counter++] = token{0, 0, output_type::argument, rpt.rep};
                    text_start = i + rpt.width;
                    break;
                case lexeme_kind::escape:
                    text_start = i + 1;
                    break;
                case lexeme_kind::wrong_syntax:
                    format_error_internal_parser_bug_report_this();
                case lexeme_kind::unclosed_brace:
                    format_error_unclosed_brace_add_closing_brace();
                case lexeme_kind::unclosed_or_opening_escape:
                    format_error_unclosed_brace_add_closing_or_escape_brace();
                case lexeme_kind::no_opening_or_escape_closed_brace:
                    format_error_no_opening_brace_add_opening_or_escape_brace();
                case lexeme_kind::contiguous_reprs:
                    format_error_contiguous_reprs_add_only_one();
                case lexeme_kind::invalid_repr:
                    format_error_invalid_repr_did_you_mean_h_or_b_or_l();
            }
            i += rpt.width;
        }
        close_text(end);
        return collection;
    }

    template<tools::fixed_string Fmt>
    struct valid_token_collection
    {
        private:
        static constexpr token_collection temp_collection{generate_token_collection(Fmt)};
        static_assert(temp_collection.counter >= 1, "valid_token_collection: Collection must have at least 1 element");

        public:
            static constexpr size_t size{temp_collection.counter};

        private:
            token entries[size];

        template<size_t... I>
        [[gnu::always_inline]]
        constexpr explicit valid_token_collection(sequence::index_list<I...>) noexcept: entries{temp_collection[I]...}
        {}

        public:
            [[gnu::always_inline]]
            constexpr valid_token_collection() noexcept: valid_token_collection(sequence::make_index_list<size>{})
            {}

            [[gnu::always_inline]]
            constexpr const token& operator[](const size_t index) const noexcept { return entries[index]; }
    };
}
