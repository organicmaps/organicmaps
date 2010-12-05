//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_NUMERIC_UTILS_FEB_23_2007_0841PM)
#define BOOST_SPIRIT_KARMA_NUMERIC_UTILS_FEB_23_2007_0841PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <limits>

#include <boost/type_traits/is_integral.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/detail/pow10.hpp>
#include <boost/spirit/home/support/detail/sign.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/string_generate.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  The value BOOST_KARMA_NUMERICS_LOOP_UNROLL specifies, how to unroll the 
//  integer string generation loop (see below).
//
//      Set the value to some integer in between 0 (no unrolling) and the 
//      largest expected generated integer string length (complete unrolling). 
//      If not specified, this value defaults to 6.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_KARMA_NUMERICS_LOOP_UNROLL)
#define BOOST_KARMA_NUMERICS_LOOP_UNROLL 6
#endif

#if BOOST_KARMA_NUMERICS_LOOP_UNROLL < 0 
#error "Please set the BOOST_KARMA_NUMERICS_LOOP_UNROLL to a non-negative value!"
#endif

namespace boost { namespace spirit { namespace karma 
{ 
    namespace detail 
    {
        ///////////////////////////////////////////////////////////////////////
        //
        //  return the absolute value from a given number, avoiding over- and 
        //  underflow
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct absolute_value_helper
        {
            typedef T result_type;
            static T call (T n)
            {
                // allow for ADL to find the correct overloads for fabs
                using namespace std;
                return fabs(n);
            }
        };

#define BOOST_SPIRIT_ABSOLUTE_VALUE(type, unsignedtype)                       \
        template <>                                                           \
        struct absolute_value_helper<type>                                    \
        {                                                                     \
            typedef unsignedtype result_type;                                 \
            static result_type call(type n)                                   \
            {                                                                 \
                return (n >= 0) ? n : (unsignedtype)(-n);                     \
            }                                                                 \
        }                                                                     \
    /**/
#define BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsignedtype)                    \
        template <>                                                           \
        struct absolute_value_helper<unsignedtype>                            \
        {                                                                     \
            typedef unsignedtype result_type;                                 \
            static result_type call(unsignedtype n)                           \
            {                                                                 \
                return n;                                                     \
            }                                                                 \
        }                                                                     \
    /**/

        BOOST_SPIRIT_ABSOLUTE_VALUE(signed char, unsigned char);
        BOOST_SPIRIT_ABSOLUTE_VALUE(char, unsigned char);
        BOOST_SPIRIT_ABSOLUTE_VALUE(short, unsigned short);
        BOOST_SPIRIT_ABSOLUTE_VALUE(int, unsigned int);
        BOOST_SPIRIT_ABSOLUTE_VALUE(long, unsigned long);
        BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned char);
        BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned short);
        BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned int);
        BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(unsigned long);
#ifdef BOOST_HAS_LONG_LONG
        BOOST_SPIRIT_ABSOLUTE_VALUE(boost::long_long_type, boost::ulong_long_type);
        BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED(boost::ulong_long_type);
#endif

#undef BOOST_SPIRIT_ABSOLUTE_VALUE
#undef BOOST_SPIRIT_ABSOLUTE_VALUE_UNSIGNED

        template <>
        struct absolute_value_helper<float>
        {
            typedef float result_type;
            static result_type call(float n)
            {
                return (spirit::detail::signbit)(n) ? -n : n;
            }
        };

        template <>
        struct absolute_value_helper<double>
        {
            typedef double result_type;
            static result_type call(double n)
            {
                return (spirit::detail::signbit)(n) ? -n : n;
            }
        };

        template <>
        struct absolute_value_helper<long double>
        {
            typedef long double result_type;
            static result_type call(long double n)
            {
                return (spirit::detail::signbit)(n) ? -n : n;
            }
        };

        // specialization for pointers
        template <typename T>
        struct absolute_value_helper<T*>
        {
            typedef std::size_t result_type;
            static std::size_t call (T* p)
            {
                return std::size_t(p);
            }
        };

        template <typename T>
        typename absolute_value_helper<T>::result_type
        absolute_value(T n)
        {
            return absolute_value_helper<T>::call(n);
        }

        ///////////////////////////////////////////////////////////////////////
        inline bool is_negative(float n) 
        { 
            return (spirit::detail::signbit)(n) ? true : false; 
        }

        inline bool is_negative(double n) 
        { 
            return (spirit::detail::signbit)(n) ? true : false; 
        }

        inline bool is_negative(long double n) 
        { 
            return (spirit::detail::signbit)(n) ? true : false; 
        }

        template <typename T>
        inline bool is_negative(T n)
        {
            return (n < 0) ? true : false;
        }

        ///////////////////////////////////////////////////////////////////////
        inline bool is_zero(float n) 
        { 
            return (math::fpclassify)(n) == FP_ZERO; 
        }

        inline bool is_zero(double n) 
        { 
            return (math::fpclassify)(n) == FP_ZERO; 
        }

        inline bool is_zero(long double n) 
        { 
            return (math::fpclassify)(n) == FP_ZERO; 
        }

        template <typename T>
        inline bool is_zero(T n)
        {
            return (n == 0) ? true : false;
        }

        ///////////////////////////////////////////////////////////////////////
        struct cast_to_long
        {
            static long call(float n, mpl::false_)
            {
                return static_cast<long>(std::floor(n));
            }

            static long call(double n, mpl::false_)
            {
                return static_cast<long>(std::floor(n));
            }

            static long call(long double n, mpl::false_)
            {
                return static_cast<long>(std::floor(n));
            }

            template <typename T>
            static long call(T n, mpl::false_)
            {
                // allow for ADL to find the correct overload for floor and 
                // lround
                using namespace std;
                return lround(floor(n));
            }

            template <typename T>
            static long call(T n, mpl::true_)
            {
                return static_cast<long>(n);
            }

            template <typename T>
            static long call(T n)
            {
                return call(n, mpl::bool_<is_integral<T>::value>());
            }
        };

        ///////////////////////////////////////////////////////////////////////
        struct truncate_to_long
        {
            static long call(float n, mpl::false_)
            {
                return is_negative(n) ? static_cast<long>(std::ceil(n)) : 
                    static_cast<long>(std::floor(n));
            }

            static long call(double n, mpl::false_)
            {
                return is_negative(n) ? static_cast<long>(std::ceil(n)) : 
                    static_cast<long>(std::floor(n));
            }

            static long call(long double n, mpl::false_)
            {
                return is_negative(n) ? static_cast<long>(std::ceil(n)) : 
                    static_cast<long>(std::floor(n));
            }

            template <typename T>
            static long call(T n, mpl::false_)
            {
                // allow for ADL to find the correct overloads for ltrunc
                using namespace std;
                return ltrunc(n);
            }

            template <typename T>
            static long call(T n, mpl::true_)
            {
                return static_cast<long>(n);
            }

            template <typename T>
            static long call(T n)
            {
                return call(n, mpl::bool_<is_integral<T>::value>());
            }
        };

        ///////////////////////////////////////////////////////////////////////
        //
        //  Traits class for radix specific number conversion
        //
        //      Convert a digit from binary representation to character 
        //      representation:
        //
        //          static int digit(unsigned n);
        //
        ///////////////////////////////////////////////////////////////////////
        template<unsigned Radix, typename CharEncoding, typename Tag>
        struct radix_traits;

        // Binary
        template<typename CharEncoding, typename Tag>
        struct radix_traits<2, CharEncoding, Tag>
        {
            static int digit(unsigned n)
            {
                return n + '0';
            }
        };

        // Octal
        template<typename CharEncoding, typename Tag>
        struct radix_traits<8, CharEncoding, Tag>
        {
            static int digit(unsigned n)
            {
                return n + '0';
            }
        };

        // Decimal 
        template<typename CharEncoding, typename Tag>
        struct radix_traits<10, CharEncoding, Tag>
        {
            static int digit(unsigned n)
            {
                return n + '0';
            }
        };

        // Hexadecimal, lower case
        template<>
        struct radix_traits<16, unused_type, unused_type>
        {
            static int digit(unsigned n)
            {
                if (n <= 9)
                    return n + '0';
                return n - 10 + 'a';
            }
        };

        // Hexadecimal, upper case
        template<typename CharEncoding, typename Tag>
        struct radix_traits<16, CharEncoding, Tag>
        {
            static int digit(unsigned n)
            {
                if (n <= 9)
                    return n + '0';

                using spirit::char_class::convert;
                return convert<CharEncoding>::to(Tag(), n - 10 + 'a');
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <unsigned Radix>
        struct divide
        {
            template <typename T>
            static T call(T& n, mpl::true_)
            {
                return n / Radix;
            }

            template <typename T>
            static T call(T& n, mpl::false_)
            {
                // Allow ADL to find the correct overload for floor
                using namespace std; 
                return floor(n / Radix);
            }

            template <typename T>
            static T call(T& n, T const&, int)
            {
                return call(n, mpl::bool_<is_integral<T>::value>());
            }

            template <typename T>
            static T call(T& n)
            {
                return call(n, mpl::bool_<is_integral<T>::value>());
            }
        };

        // specialization for division by 10
        template <>
        struct divide<10>
        {
            template <typename T>
            static T call(T& n, T, int, mpl::true_)
            {
                return n / 10;
            }

            template <typename T>
            static T call(T, T& num, int exp, mpl::false_)
            {
                // Allow ADL to find the correct overload for floor
                using namespace std; 
                return floor(num / spirit::detail::pow10<T>(exp));
            }

            template <typename T>
            static T call(T& n, T& num, int exp)
            {
                return call(n, num, exp, mpl::bool_<is_integral<T>::value>());
            }

            template <typename T>
            static T call(T& n)
            {
                return call(n, n, 1, mpl::bool_<is_integral<T>::value>());
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <unsigned Radix>
        struct remainder
        {
            template <typename T>
            static long call(T n, mpl::true_)
            {
                // this cast is safe since we know the result is not larger 
                // than Radix
                return static_cast<long>(n % Radix);
            }

            template <typename T>
            static long call(T n, mpl::false_)
            {
                // Allow ADL to find the correct overload for fmod
                using namespace std; 
                return cast_to_long::call(fmod(n, T(Radix)));
            }

            template <typename T>
            static long call(T n)
            {
                return call(n, mpl::bool_<is_integral<T>::value>());
            }
        };

    }   // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The int_inserter template takes care of the integer to string 
    //  conversion. If specified, the loop is unrolled for better performance.
    //
    //      Set the value BOOST_KARMA_NUMERICS_LOOP_UNROLL to some integer in 
    //      between 0 (no unrolling) and the largest expected generated integer 
    //      string length (complete unrolling). 
    //      If not specified, this value defaults to 6.
    //
    ///////////////////////////////////////////////////////////////////////////
#define BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX(z, x, data)                    \
        if (!detail::is_zero(n)) {                                            \
            int ch = radix_type::digit(remainder_type::call(n));              \
            n = divide_type::call(n, num, ++exp);                             \
    /**/

#define BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX(z, x, data)                    \
            *sink = char(ch);                                                 \
            ++sink;                                                           \
        }                                                                     \
    /**/

    template <
        unsigned Radix, typename CharEncoding = unused_type
      , typename Tag = unused_type>
    struct int_inserter
    {
        typedef detail::radix_traits<Radix, CharEncoding, Tag> radix_type;
        typedef detail::divide<Radix> divide_type;
        typedef detail::remainder<Radix> remainder_type;

        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T n, T& num, int exp)
        {
            // remainder_type::call returns n % Radix
            int ch = radix_type::digit(remainder_type::call(n));
            n = divide_type::call(n, num, ++exp);

            BOOST_PP_REPEAT(
                BOOST_KARMA_NUMERICS_LOOP_UNROLL,
                BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX, _);

            if (!detail::is_zero(n)) 
                call(sink, n, num, exp);

            BOOST_PP_REPEAT(
                BOOST_KARMA_NUMERICS_LOOP_UNROLL,
                BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX, _);

            *sink = char(ch);
            ++sink;
            return true;
        }

        //  Common code for integer string representations
        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T n)
        {
            return call(sink, n, n, 0);
        }

    private:
        // helper function returning the biggest number representable either in
        // a boost::long_long_type (if this does exist) or in a plain long
        // otherwise
#if defined(BOOST_HAS_LONG_LONG)
        typedef boost::long_long_type biggest_long_type;
#else
        typedef long biggest_long_type;
#endif

        static biggest_long_type max_long()
        {
            return (std::numeric_limits<biggest_long_type>::max)();
        }

    public:
        // Specialization for doubles and floats, falling back to long integers 
        // for representable values. These specializations speed up formatting
        // of floating point numbers considerably as all the required 
        // arithmetics will be executed using integral data types.
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, long double n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, double n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, float n)
        {
            if (std::fabs(n) < max_long())
            {
                biggest_long_type l((biggest_long_type)n);
                return call(sink, l, l, 0);
            }
            return call(sink, n, n, 0);
        }
    };

#undef BOOST_KARMA_NUMERICS_INNER_LOOP_PREFIX
#undef BOOST_KARMA_NUMERICS_INNER_LOOP_SUFFIX

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The uint_inserter template takes care of the conversion of any integer 
    //  to a string, while interpreting the number as an unsigned type.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        unsigned Radix, typename CharEncoding = unused_type
      , typename Tag = unused_type>
    struct uint_inserter : int_inserter<Radix, CharEncoding, Tag>
    {
        typedef int_inserter<Radix, CharEncoding, Tag> base_type;

        //  Common code for integer string representations
        template <typename OutputIterator, typename T>
        static bool
        call(OutputIterator& sink, T const& n)
        {
            typedef typename detail::absolute_value_helper<T>::result_type type;
            type un = type(n);
            return base_type::call(sink, un, un, 0);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The sign_inserter template generates a sign for a given numeric value.
    //
    //    The parameter forcesign allows to generate a sign even for positive  
    //    numbers.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct sign_inserter
    {
        template <typename OutputIterator>
        static bool
        call_noforce(OutputIterator& sink, bool /*is_zero*/, bool is_negative)
        {
            // generate a sign for negative numbers only
            if (is_negative) {
                *sink = '-';
                ++sink;
            }
            return true;
        }

        template <typename OutputIterator>
        static bool
        call_force(OutputIterator& sink, bool is_zero, bool is_negative)
        {
            // generate a sign for all numbers except zero
            if (!is_zero) 
                *sink = is_negative ? '-' : '+';
            else 
                *sink = ' ';

            ++sink;
            return true;
        }

        template <typename OutputIterator>
        static bool
        call(OutputIterator& sink, bool is_zero, bool is_negative
          , bool forcesign)
        {
            return forcesign ?
                call_force(sink, is_zero, is_negative) :
                call_noforce(sink, is_zero, is_negative);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  These are helper functions for the real policies allowing to generate
    //  a single character and a string
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding = unused_type, typename Tag = unused_type>
    struct char_inserter
    {
        template <typename OutputIterator, typename Char>
        static bool call(OutputIterator& sink, Char c)
        {
            return detail::generate_to(sink, c, CharEncoding(), Tag());
        }
    };

    template <typename CharEncoding = unused_type, typename Tag = unused_type>
    struct string_inserter
    {
        template <typename OutputIterator, typename String>
        static bool call(OutputIterator& sink, String str)
        {
            return detail::string_generate(sink, str, CharEncoding(), Tag());
        }
    };

}}}

#endif

