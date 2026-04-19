#pragma once

namespace kernel
{
    template<typename T>
    struct is_integral
    {
        static constexpr bool value = false;
    };

    template<>
    struct is_integral<bool>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<char>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<signed char>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<unsigned char>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<short>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<unsigned short>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<int>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<unsigned int>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<long>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<unsigned long>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<long long>
    {
        static constexpr bool value = true;
    };

    template<>
    struct is_integral<unsigned long long>
    {
        static constexpr bool value = true;
    };

    template<typename T>
    inline constexpr bool is_integral_v = is_integral<T>::value;
}