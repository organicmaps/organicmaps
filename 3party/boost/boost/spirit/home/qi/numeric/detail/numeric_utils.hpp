/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2006 Stephen Nutt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_NUMERIC_UTILS_APRIL_17_2006_0816AM)
#define SPIRIT_NUMERIC_UTILS_APRIL_17_2006_0816AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/detail/iterator.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/char_encoding/ascii.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>

#include <limits>
#include <boost/limits.hpp>

#if !defined(SPIRIT_NUMERICS_LOOP_UNROLL)
# define SPIRIT_NUMERICS_LOOP_UNROLL 3
#endif

namespace boost { namespace spirit { namespace qi { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  Traits class for radix specific number conversion
    //
    //      Test the validity of a single character:
    //
    //          template<typename Char> static bool is_valid(Char ch);
    //
    //      Convert a digit from character representation to binary
    //      representation:
    //
    //          template<typename Char> static int digit(Char ch);
    //
    //      The maximum radix digits that can be represented without
    //      overflow:
    //
    //          template<typename T> struct digits::value;
    //
    ///////////////////////////////////////////////////////////////////////////
    template <unsigned Radix>
    struct radix_traits;

    // Binary
    template <>
    struct radix_traits<2>
    {
        template<typename Char>
        static bool is_valid(Char ch)
        {
            return ('0' == ch || '1' == ch);
        }

        template<typename Char>
        static unsigned digit(Char ch)
        {
            return ch - '0';
        }

        template<typename T>
        struct digits
        {
            typedef std::numeric_limits<T> numeric_limits_;
            BOOST_STATIC_CONSTANT(int, value = numeric_limits_::digits);
        };
    };

    // Octal
    template <>
    struct radix_traits<8>
    {
        template<typename Char>
        static bool is_valid(Char ch)
        {
            return ch >= '0' && ch <= '7';
        }

        template<typename Char>
        static unsigned digit(Char ch)
        {
            return ch - '0';
        }

        template<typename T>
        struct digits
        {
            typedef std::numeric_limits<T> numeric_limits_;
            BOOST_STATIC_CONSTANT(int, value = numeric_limits_::digits / 3);
        };
    };

    // Decimal
    template <>
    struct radix_traits<10>
    {
        template<typename Char>
        static bool is_valid(Char ch)
        {
            return ch >= '0' && ch <= '9';
        }

        template<typename Char>
        static unsigned digit(Char ch)
        {
            return ch - '0';
        }

        template<typename T>
        struct digits
        {
            typedef std::numeric_limits<T> numeric_limits_;
            BOOST_STATIC_CONSTANT(int, value = numeric_limits_::digits10);
        };
    };

    // Hexadecimal
    template <>
    struct radix_traits<16>
    {
        template<typename Char>
        static bool is_valid(Char ch)
        {
            return (ch >= '0' && ch <= '9')
            || (ch >= 'a' && ch <= 'f')
            || (ch >= 'A' && ch <= 'F');
        }

        template<typename Char>
        static unsigned digit(Char ch)
        {
            if (ch >= '0' && ch <= '9')
                return ch - '0';
            return spirit::char_encoding::ascii::tolower(ch) - 'a' + 10;
        }

        template<typename T>
        struct digits
        {
            typedef std::numeric_limits<T> numeric_limits_;
            BOOST_STATIC_CONSTANT(int, value = numeric_limits_::digits / 4);
        };
    };

    ///////////////////////////////////////////////////////////////////////////
    //  positive_accumulator/negative_accumulator: Accumulator policies for
    //  extracting integers. Use positive_accumulator if number is positive.
    //  Use negative_accumulator if number is negative.
    ///////////////////////////////////////////////////////////////////////////
    template <unsigned Radix>
    struct positive_accumulator
    {
        template <typename T, typename Char>
        static void add(T& n, Char ch, mpl::false_) // unchecked add
        {
            const int digit = radix_traits<Radix>::digit(ch);
            n = n * T(Radix) + T(digit);
        }

        template <typename T, typename Char>
        static bool add(T& n, Char ch, mpl::true_) // checked add
        {
            // Ensure n *= Radix will not overflow
            static T const max = (std::numeric_limits<T>::max)();
            static T const val = (max - 1) / Radix;
            if (n > val)
                return false;

            n *= Radix;

            // Ensure n += digit will not overflow
            const int digit = radix_traits<Radix>::digit(ch);
            if (n > max - digit)
                return false;

            n += static_cast<T>(digit);
            return true;
        }
    };

    template <unsigned Radix>
    struct negative_accumulator
    {
        template <typename T, typename Char>
        static void add(T& n, Char ch, mpl::false_) // unchecked subtract
        {
            const int digit = radix_traits<Radix>::digit(ch);
            n = n * T(Radix) - T(digit);
        }

        template <typename T, typename Char>
        static bool add(T& n, Char ch, mpl::true_) // checked subtract
        {
            // Ensure n *= Radix will not underflow
            static T const min = (std::numeric_limits<T>::min)();
            static T const val = (min + 1) / T(Radix);
            if (n < val)
                return false;

            n *= Radix;

            // Ensure n -= digit will not underflow
            int const digit = radix_traits<Radix>::digit(ch);
            if (n < min + digit)
                return false;

            n -= static_cast<T>(digit);
            return true;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Common code for extract_int::parse specializations
    ///////////////////////////////////////////////////////////////////////////
    template <unsigned Radix, typename Accumulator, int MaxDigits>
    struct int_extractor
    {
        template <typename Char, typename T>
        static bool
        call(Char ch, std::size_t count, T& n, mpl::true_)
        {
            static std::size_t const
                overflow_free = radix_traits<Radix>::template digits<T>::value - 1;

            if (count < overflow_free)
            {
                Accumulator::add(n, ch, mpl::false_());
            }
            else
            {
                if (!Accumulator::add(n, ch, mpl::true_()))
                    return false; //  over/underflow!
            }
            return true;
        }

        template <typename Char, typename T>
        static bool
        call(Char ch, std::size_t /*count*/, T& n, mpl::false_)
        {
            // no need to check for overflow
            Accumulator::add(n, ch, mpl::false_());
            return true;
        }

        template <typename Char>
        static bool
        call(Char /*ch*/, std::size_t /*count*/, unused_type, mpl::false_)
        {
            return true;
        }

        template <typename Char, typename T>
        static bool
        call(Char ch, std::size_t count, T& n)
        {
            return call(ch, count, n
              , mpl::bool_<
                    (   (MaxDigits < 0)
                    ||  (MaxDigits > radix_traits<Radix>::template digits<T>::value)
                    )
                  && std::numeric_limits<T>::is_modulo
                >()
            );
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  End of loop checking: check if the number of digits
    //  being parsed exceeds MaxDigits. Note: if MaxDigits == -1
    //  we don't do any checking.
    ///////////////////////////////////////////////////////////////////////////
    template <int MaxDigits>
    struct check_max_digits
    {
        static bool
        call(std::size_t count)
        {
            return count < MaxDigits; // bounded
        }
    };

    template <>
    struct check_max_digits<-1>
    {
        static bool
        call(std::size_t /*count*/)
        {
            return true; // unbounded
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  extract_int: main code for extracting integers
    ///////////////////////////////////////////////////////////////////////////
#define SPIRIT_NUMERIC_INNER_LOOP(z, x, data)                                   \
        if (!check_max_digits<MaxDigits>::call(count + leading_zeros)           \
            || it == last)                                                      \
            break;                                                              \
        ch = *it;                                                               \
        if (!radix_check::is_valid(ch) || !extractor::call(ch, count, val))     \
            break;                                                              \
        ++it;                                                                   \
        ++count;                                                                \
    /**/

    template <
        typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
      , typename Accumulator = positive_accumulator<Radix>
      , bool Accumulate = false
    >
    struct extract_int
    {
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(push)
# pragma warning(disable: 4127)   // conditional expression is constant
#endif
        template <typename Iterator, typename Attribute>
        static bool
        parse_main(
            Iterator& first
          , Iterator const& last
          , Attribute& attr)
        {
            typedef radix_traits<Radix> radix_check;
            typedef int_extractor<Radix, Accumulator, MaxDigits> extractor;
            typedef typename
                boost::detail::iterator_traits<Iterator>::value_type
            char_type;

            Iterator it = first;
            std::size_t leading_zeros = 0;
            if (!Accumulate)
            {
                // skip leading zeros
                while (it != last && *it == '0' && leading_zeros < MaxDigits)
                {
                    ++it;
                    ++leading_zeros;
                }
            }

            typedef typename
                traits::attribute_type<Attribute>::type
            attribute_type;

            attribute_type val = Accumulate ? attr : attribute_type(0);
            std::size_t count = 0;
            char_type ch;

            while (true)
            {
                BOOST_PP_REPEAT(
                    SPIRIT_NUMERICS_LOOP_UNROLL
                  , SPIRIT_NUMERIC_INNER_LOOP, _)
            }

            if (count + leading_zeros >= MinDigits)
            {
                traits::assign_to(val, attr);
                first = it;
                return true;
            }
            return false;
        }
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif

        template <typename Iterator>
        static bool
        parse(
            Iterator& first
          , Iterator const& last
          , unused_type)
        {
            T n = 0; // must calculate value to detect over/underflow
            return parse_main(first, last, n);
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse(
            Iterator& first
          , Iterator const& last
          , Attribute& attr)
        {
            return parse_main(first, last, attr);
        }
    };
#undef SPIRIT_NUMERIC_INNER_LOOP

    ///////////////////////////////////////////////////////////////////////////
    //  extract_int: main code for extracting integers
    //  common case where MinDigits == 1 and MaxDigits = -1
    ///////////////////////////////////////////////////////////////////////////
#define SPIRIT_NUMERIC_INNER_LOOP(z, x, data)                                   \
        if (it == last)                                                         \
            break;                                                              \
        ch = *it;                                                               \
        if (!radix_check::is_valid(ch))                                         \
            break;                                                              \
        if (!extractor::call(ch, count, val))                                   \
            return false;                                                       \
        ++it;                                                                   \
        ++count;                                                                \
    /**/

    template <typename T, unsigned Radix, typename Accumulator, bool Accumulate>
    struct extract_int<T, Radix, 1, -1, Accumulator, Accumulate>
    {
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(push)
# pragma warning(disable: 4127)   // conditional expression is constant
#endif
        template <typename Iterator, typename Attribute>
        static bool
        parse_main(
            Iterator& first
          , Iterator const& last
          , Attribute& attr)
        {
            typedef radix_traits<Radix> radix_check;
            typedef int_extractor<Radix, Accumulator, -1> extractor;
            typedef typename
                boost::detail::iterator_traits<Iterator>::value_type
            char_type;

            Iterator it = first;
            std::size_t count = 0;
            if (!Accumulate)
            {
                // skip leading zeros
                while (it != last && *it == '0')
                {
                    ++it;
                    ++count;
                }

                if (it == last)
                {
                    if (count == 0) // must have at least one digit
                        return false;
                    traits::assign_to(0, attr);
                    first = it;
                    return true;
                }
            }

            typedef typename
                traits::attribute_type<Attribute>::type
            attribute_type;

            attribute_type val = Accumulate ? attr : attribute_type(0);
            char_type ch = *it;

            if (!radix_check::is_valid(ch) || !extractor::call(ch, 0, val))
            {
                if (count == 0) // must have at least one digit
                    return false;
                traits::assign_to(val, attr);
                first = it;
                return true;
            }

            count = 0;
            ++it;
            while (true)
            {
                BOOST_PP_REPEAT(
                    SPIRIT_NUMERICS_LOOP_UNROLL
                  , SPIRIT_NUMERIC_INNER_LOOP, _)
            }

            traits::assign_to(val, attr);
            first = it;
            return true;
        }
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif

        template <typename Iterator>
        static bool
        parse(
            Iterator& first
          , Iterator const& last
          , unused_type)
        {
            T n = 0; // must calculate value to detect over/underflow
            return parse_main(first, last, n);
        }

        template <typename Iterator, typename Attribute>
        static bool
        parse(
            Iterator& first
          , Iterator const& last
          , Attribute& attr)
        {
            return parse_main(first, last, attr);
        }
    };

#undef SPIRIT_NUMERIC_INNER_LOOP

    ///////////////////////////////////////////////////////////////////////////
    // Cast an signed integer to an unsigned integer
    ///////////////////////////////////////////////////////////////////////////
    template <typename T,
        bool force_unsigned
            = mpl::and_<is_integral<T>, is_signed<T> >::value>
    struct cast_unsigned;

    template <typename T>
    struct cast_unsigned<T, true>
    {
        typedef typename make_unsigned<T>::type unsigned_type;
        typedef typename make_unsigned<T>::type& unsigned_type_ref;

        static unsigned_type_ref call(T& n)
        {
            return unsigned_type_ref(n);
        }
    };

    template <typename T>
    struct cast_unsigned<T, false>
    {
        static T& call(T& n)
        {
            return n;
        }
    };

}}}}

#endif
