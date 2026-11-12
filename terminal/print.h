#pragma once

#include <stddef.h>
#include "tools/fixed_string.h"
#include "internals/format_parse.h"
#include "tools/types.h"
#include "cpu/interrupt_guard.h"
#include "tools/sequence.h"
#include "terminal/output.h"

namespace terminal
{

    template<tools::fixed_string Fmt, typename... Args>
    [[gnu::regparm(3)]]
    void print(Args... values) noexcept
    {
        using tkn_collection = format_parser::valid_token_collection<Fmt>;
        constexpr size_t total_args{sizeof...(Args)};
        static_assert(tkn_collection::total_arguments == total_args, "number of placeholders does not match the amount of arguments given");
        cpu::interrupt_guard guard{};

        [&]<size_t... I>(sequence::index_list<I...>)
        {
            auto step = [&]<size_t J>(sequence::index_list<J>)
            {
                constexpr format_parser::token t{tkn_collection{}[J]};
                if constexpr(t.o_type == format_parser::output_type::plain_text)
                {

                }
                else
                {

                }
            };

            (step(sequence::index_list<I>{}), ...);
        }(sequence::make_index_list<tkn_collection::size>{});
    }
}
