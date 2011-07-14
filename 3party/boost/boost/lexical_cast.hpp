#ifndef BOOST_LEXICAL_CAST_INCLUDED
#define BOOST_LEXICAL_CAST_INCLUDED

// Boost lexical_cast.hpp header  -------------------------------------------//
//
// See http://www.boost.org/libs/conversion for documentation.
// See end of this header for rights and permissions.
//
// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Slettebo,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        Alexander Nasonov, Antony Polukhin and other Boosters
// when:  November 2000, March 2003, June 2005, June 2006, March 2011

#include <climits>
#include <cstddef>
#include <istream>
#include <string>
#include <typeinfo>
#include <exception>
#include <cmath>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/mpl/if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/ice.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/call_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/lcast_precision.hpp>
#include <boost/detail/workaround.hpp>

#ifndef BOOST_NO_STD_LOCALE
#include <locale>
#endif

#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#else
#include <sstream>
#endif

#if defined(BOOST_NO_STRINGSTREAM) || defined(BOOST_NO_STD_WSTRING)
#define BOOST_LCAST_NO_WCHAR_T
#endif

#ifdef BOOST_NO_TYPEID
#define BOOST_LCAST_THROW_BAD_CAST(S, T) throw_exception(bad_lexical_cast())
#else
#define BOOST_LCAST_THROW_BAD_CAST(Source, Target) \
    throw_exception(bad_lexical_cast(typeid(Source), typeid(Target)))
#endif

namespace boost
{
    // exception used to indicate runtime lexical_cast failure
    class bad_lexical_cast :
    // workaround MSVC bug with std::bad_cast when _HAS_EXCEPTIONS == 0 
#if defined(BOOST_MSVC) && defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS 
        public std::exception 
#else 
        public std::bad_cast 
#endif 

#if defined(__BORLANDC__) && BOOST_WORKAROUND( __BORLANDC__, < 0x560 )
        // under bcc32 5.5.1 bad_cast doesn't derive from exception
        , public std::exception
#endif

    {
    public:
        bad_lexical_cast() :
#ifndef BOOST_NO_TYPEID
          source(&typeid(void)), target(&typeid(void))
#else
          source(0), target(0) // this breaks getters
#endif
        {
        }

        bad_lexical_cast(
            const std::type_info &source_type_arg,
            const std::type_info &target_type_arg) :
            source(&source_type_arg), target(&target_type_arg)
        {
        }

        const std::type_info &source_type() const
        {
            return *source;
        }
        const std::type_info &target_type() const
        {
            return *target;
        }

        virtual const char *what() const throw()
        {
            return "bad lexical cast: "
                   "source type value could not be interpreted as target";
        }
        virtual ~bad_lexical_cast() throw()
        {
        }
    private:
        const std::type_info *source;
        const std::type_info *target;
    };

    namespace detail // selectors for choosing stream character type
    {
        template<typename Type>
        struct stream_char
        {
            typedef char type;
        };

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<class CharT, class Traits, class Alloc>
        struct stream_char< std::basic_string<CharT,Traits,Alloc> >
        {
            typedef CharT type;
        };
#endif

#ifndef BOOST_LCAST_NO_WCHAR_T
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
        template<>
        struct stream_char<wchar_t>
        {
            typedef wchar_t type;
        };
#endif

        template<>
        struct stream_char<wchar_t *>
        {
            typedef wchar_t type;
        };

        template<>
        struct stream_char<const wchar_t *>
        {
            typedef wchar_t type;
        };

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<>
        struct stream_char<std::wstring>
        {
            typedef wchar_t type;
        };
#endif
#endif

        template<typename TargetChar, typename SourceChar>
        struct widest_char
        {
            typedef TargetChar type;
        };

        template<>
        struct widest_char<char, wchar_t>
        {
            typedef wchar_t type;
        };
    }

    namespace detail // deduce_char_traits template
    {
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<class CharT, class Target, class Source>
        struct deduce_char_traits
        {
            typedef std::char_traits<CharT> type;
        };

        template<class CharT, class Traits, class Alloc, class Source>
        struct deduce_char_traits< CharT
                                 , std::basic_string<CharT,Traits,Alloc>
                                 , Source
                                 >
        {
            typedef Traits type;
        };

        template<class CharT, class Target, class Traits, class Alloc>
        struct deduce_char_traits< CharT
                                 , Target
                                 , std::basic_string<CharT,Traits,Alloc>
                                 >
        {
            typedef Traits type;
        };

        template<class CharT, class Traits, class Alloc1, class Alloc2>
        struct deduce_char_traits< CharT
                                 , std::basic_string<CharT,Traits,Alloc1>
                                 , std::basic_string<CharT,Traits,Alloc2>
                                 >
        {
            typedef Traits type;
        };
#endif
    }

    namespace detail // lcast_src_length
    {
        // Return max. length of string representation of Source;
        // 0 if unlimited (with exceptions for some types, see below).
        // Values with limited string representation are placed to
        // the buffer locally defined in lexical_cast function.
        // 1 is returned for few types such as CharT const* or
        // std::basic_string<CharT> that already have an internal
        // buffer ready to be reused by lexical_stream_limited_src.
        // Each specialization should have a correspondent operator<<
        // defined in lexical_stream_limited_src.
        template< class CharT  // A result of widest_char transformation.
                , class Source // Source type of lexical_cast.
                >
        struct lcast_src_length
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 0);
            // To check coverage, build the test with
            // bjam --v2 profile optimization=off
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char, bool>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char, char>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char, signed char>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
        template<>
        struct lcast_src_length<char, unsigned char>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
        template<>
        struct lcast_src_length<char, signed char*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
        template<>
        struct lcast_src_length<char, unsigned char*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
        template<>
        struct lcast_src_length<char, signed char const*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
        template<>
        struct lcast_src_length<char, unsigned char const*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<>
        struct lcast_src_length<wchar_t, bool>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<wchar_t, char>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
        template<>
        struct lcast_src_length<wchar_t, wchar_t>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
#endif
#endif

        template<>
        struct lcast_src_length<char, char const*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char, char*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<>
        struct lcast_src_length<wchar_t, wchar_t const*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<wchar_t, wchar_t*>
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
#endif

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<class CharT, class Traits, class Alloc>
        struct lcast_src_length< CharT, std::basic_string<CharT,Traits,Alloc> >
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
#else
        template<>
        struct lcast_src_length< char, std::basic_string<char> >
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<>
        struct lcast_src_length< wchar_t, std::basic_string<wchar_t> >
        {
            BOOST_STATIC_CONSTANT(std::size_t, value = 1);
            static void check_coverage() {}
        };
#endif
#endif

        // Helper for integral types.
        // Notes on length calculation:
        // Max length for 32bit int with grouping "\1" and thousands_sep ',':
        // "-2,1,4,7,4,8,3,6,4,7"
        //  ^                    - is_signed
        //   ^                   - 1 digit not counted by digits10
        //    ^^^^^^^^^^^^^^^^^^ - digits10 * 2
        //
        // Constant is_specialized is used instead of constant 1
        // to prevent buffer overflow in a rare case when
        // <boost/limits.hpp> doesn't add missing specialization for
        // numeric_limits<T> for some integral type T.
        // When is_specialized is false, the whole expression is 0.
        template<class Source>
        struct lcast_src_length_integral
        {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
            BOOST_STATIC_CONSTANT(std::size_t, value =
                  std::numeric_limits<Source>::is_signed +
                  std::numeric_limits<Source>::is_specialized + // == 1
                  std::numeric_limits<Source>::digits10 * 2
              );
#else
            BOOST_STATIC_CONSTANT(std::size_t, value = 156);
            BOOST_STATIC_ASSERT(sizeof(Source) * CHAR_BIT <= 256);
#endif
        };

#define BOOST_LCAST_DEF1(CharT, T)               \
    template<> struct lcast_src_length<CharT, T> \
        : lcast_src_length_integral<T>           \
    { static void check_coverage() {} };

#ifdef BOOST_LCAST_NO_WCHAR_T
#define BOOST_LCAST_DEF(T) BOOST_LCAST_DEF1(char, T)
#else
#define BOOST_LCAST_DEF(T)          \
        BOOST_LCAST_DEF1(char, T)   \
        BOOST_LCAST_DEF1(wchar_t, T)
#endif

        BOOST_LCAST_DEF(short)
        BOOST_LCAST_DEF(unsigned short)
        BOOST_LCAST_DEF(int)
        BOOST_LCAST_DEF(unsigned int)
        BOOST_LCAST_DEF(long)
        BOOST_LCAST_DEF(unsigned long)
#if defined(BOOST_HAS_LONG_LONG)
        BOOST_LCAST_DEF(boost::ulong_long_type)
        BOOST_LCAST_DEF(boost::long_long_type )
#elif defined(BOOST_HAS_MS_INT64)
        BOOST_LCAST_DEF(unsigned __int64)
        BOOST_LCAST_DEF(         __int64)
#endif

#undef BOOST_LCAST_DEF
#undef BOOST_LCAST_DEF1

#ifndef BOOST_LCAST_NO_COMPILE_TIME_PRECISION
        // Helper for floating point types.
        // -1.23456789e-123456
        // ^                   sign
        //  ^                  leading digit
        //   ^                 decimal point 
        //    ^^^^^^^^         lcast_precision<Source>::value
        //            ^        "e"
        //             ^       exponent sign
        //              ^^^^^^ exponent (assumed 6 or less digits)
        // sign + leading digit + decimal point + "e" + exponent sign == 5
        template<class Source>
        struct lcast_src_length_floating
        {
            BOOST_STATIC_ASSERT(
                    std::numeric_limits<Source>::max_exponent10 <=  999999L &&
                    std::numeric_limits<Source>::min_exponent10 >= -999999L
                );
            BOOST_STATIC_CONSTANT(std::size_t, value =
                    5 + lcast_precision<Source>::value + 6
                );
        };

        template<>
        struct lcast_src_length<char,float>
          : lcast_src_length_floating<float>
        {
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char,double>
          : lcast_src_length_floating<double>
        {
            static void check_coverage() {}
        };

        template<>
        struct lcast_src_length<char,long double>
          : lcast_src_length_floating<long double>
        {
            static void check_coverage() {}
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
    template<>
    struct lcast_src_length<wchar_t,float>
      : lcast_src_length_floating<float>
    {
        static void check_coverage() {}
    };

    template<>
    struct lcast_src_length<wchar_t,double>
      : lcast_src_length_floating<double>
    {
        static void check_coverage() {}
    };

    template<>
    struct lcast_src_length<wchar_t,long double>
      : lcast_src_length_floating<long double>
    {
        static void check_coverage() {}
    };

#endif // #ifndef BOOST_LCAST_NO_WCHAR_T
#endif // #ifndef BOOST_LCAST_NO_COMPILE_TIME_PRECISION
    }

    namespace detail // '0', '+' and '-' constants
    {
        template<typename CharT> struct lcast_char_constants;

        template<>
        struct lcast_char_constants<char>
        {
            BOOST_STATIC_CONSTANT(char, zero  = '0');
            BOOST_STATIC_CONSTANT(char, minus = '-');
            BOOST_STATIC_CONSTANT(char, plus = '+');
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<>
        struct lcast_char_constants<wchar_t>
        {
            BOOST_STATIC_CONSTANT(wchar_t, zero  = L'0');
            BOOST_STATIC_CONSTANT(wchar_t, minus = L'-');
            BOOST_STATIC_CONSTANT(wchar_t, plus = L'+');
        };
#endif
    }

    namespace detail // lexical_streambuf_fake
    {
        struct lexical_streambuf_fake
        {
        };
    }

    namespace detail // lcast_to_unsigned
    {
#if (defined _MSC_VER)
# pragma warning( push )
// C4146: unary minus operator applied to unsigned type, result still unsigned
# pragma warning( disable : 4146 )
#elif defined( __BORLANDC__ )
# pragma option push -w-8041
#endif
        template<class T>
        inline
        BOOST_DEDUCED_TYPENAME make_unsigned<T>::type lcast_to_unsigned(T value)
        {
            typedef BOOST_DEDUCED_TYPENAME make_unsigned<T>::type result_type;
            result_type uvalue = static_cast<result_type>(value);
            return value < 0 ? -uvalue : uvalue;
        }
#if (defined _MSC_VER)
# pragma warning( pop )
#elif defined( __BORLANDC__ )
# pragma option pop
#endif
    }

    namespace detail // lcast_put_unsigned
    {
        template<class Traits, class T, class CharT>
        CharT* lcast_put_unsigned(T n, CharT* finish)
        {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
            BOOST_STATIC_ASSERT(!std::numeric_limits<T>::is_signed);
#endif

            typedef typename Traits::int_type int_type;
            CharT const czero = lcast_char_constants<CharT>::zero;
            int_type const zero = Traits::to_int_type(czero);

#ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
            // TODO: use BOOST_NO_STD_LOCALE
            std::locale loc;
            typedef std::numpunct<CharT> numpunct;
            numpunct const& np = BOOST_USE_FACET(numpunct, loc);
            std::string const& grouping = np.grouping();
            std::string::size_type const grouping_size = grouping.size();

            if ( grouping_size && grouping[0] > 0 )
            {

#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
            // Check that ulimited group is unreachable:
            BOOST_STATIC_ASSERT(std::numeric_limits<T>::digits10 < CHAR_MAX);
#endif
                CharT thousands_sep = np.thousands_sep();
                std::string::size_type group = 0; // current group number
                char last_grp_size = grouping[0];
                char left = last_grp_size;

                do
                {
                    if(left == 0)
                    {
                        ++group;
                        if(group < grouping_size)
                        {
                            char const grp_size = grouping[group];
                            last_grp_size = grp_size <= 0 ? CHAR_MAX : grp_size;
                        }

                        left = last_grp_size;
                        --finish;
                        Traits::assign(*finish, thousands_sep);
                    }

                    --left;

                    --finish;
                    int_type const digit = static_cast<int_type>(n % 10U);
                    Traits::assign(*finish, Traits::to_char_type(zero + digit));
                    n /= 10;
                } while(n);

            } else
#endif
            {
                do
                {
                    --finish;
                    int_type const digit = static_cast<int_type>(n % 10U);
                    Traits::assign(*finish, Traits::to_char_type(zero + digit));
                    n /= 10;
                } while(n);
            }

            return finish;
        }
    }

    namespace detail // lcast_ret_unsigned
    {
        template<class Traits, class T, class CharT>
        inline bool lcast_ret_unsigned(T& value, const CharT* const begin, const CharT* end)
        {
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
            BOOST_STATIC_ASSERT(!std::numeric_limits<T>::is_signed);
#endif
            typedef typename Traits::int_type int_type;
            CharT const czero = lcast_char_constants<CharT>::zero;
            --end;
            value = 0;

            if ( *end < czero || *end >= czero + 10 || begin > end)
                return false;
            value = *end - czero;
            --end;
            T multiplier = 1;

#ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
            // TODO: use BOOST_NO_STD_LOCALE
            std::locale loc;
            typedef std::numpunct<CharT> numpunct;
            numpunct const& np = BOOST_USE_FACET(numpunct, loc);
            std::string const& grouping = np.grouping();
            std::string::size_type const grouping_size = grouping.size();

            /* According to [22.2.2.1.2] of Programming languages - C++
             * we MUST check for correct grouping
             */
            if (grouping_size && grouping[0] > 0)
            {
                unsigned char current_grouping = 0;
                CharT const thousands_sep = np.thousands_sep();
                char remained = grouping[current_grouping] - 1;
                bool shall_we_return = true;

                for(;end>=begin; --end)
                {
                    if (remained) {
                        T const new_sub_value = multiplier * 10 * (*end - czero);

                        if (*end < czero || *end >= czero + 10
                                /* detecting overflow */
                                || new_sub_value/10 != multiplier * (*end - czero)
                                || static_cast<T>((std::numeric_limits<T>::max)()-new_sub_value) < value
                                )
                            return false;

                        value += new_sub_value;
                        multiplier *= 10;
                        --remained;
                    } else {
                        if ( !Traits::eq(*end, thousands_sep) ) //|| begin == end ) return false;
                        {
                            /*
                             * According to Programming languages - C++
                             * Digit grouping is checked. That is, the positions of discarded
                             * separators is examined for consistency with
                             * use_facet<numpunct<charT> >(loc ).grouping()
                             *
                             * BUT what if there is no separators at all and grouping()
                             * is not empty? Well, we have no extraced separators, so we
                             * won`t check them for consistency. This will allow us to
                             * work with "C" locale from other locales
                             */
                            shall_we_return = false;
                            break;
                        } else {
                            if ( begin == end ) return false;
                            if (current_grouping < grouping_size-1 ) ++current_grouping;
                            remained = grouping[current_grouping];
                        }
                    }
                }

                if (shall_we_return) return true;
            }
#endif
            {
                while ( begin <= end )
                {
                    T const new_sub_value = multiplier * 10 * (*end - czero);

                    if (*end < czero || *end >= czero + 10
                            /* detecting overflow */
                            || new_sub_value/10 != multiplier * (*end - czero)
                            || static_cast<T>((std::numeric_limits<T>::max)()-new_sub_value) < value
                            )
                        return false;

                    value += new_sub_value;
                    multiplier *= 10;
                    --end;
                }
            }
            return true;
        }
    }

    namespace detail // stream wrapper for handling lexical conversions
    {
        template<typename Target, typename Source, typename Traits>
        class lexical_stream
        {
        private:
            typedef typename widest_char<
                typename stream_char<Target>::type,
                typename stream_char<Source>::type>::type char_type;

            typedef Traits traits_type;

        public:
            lexical_stream(char_type* = 0, char_type* = 0)
            {
                stream.unsetf(std::ios::skipws);
                lcast_set_precision(stream, static_cast<Source*>(0), static_cast<Target*>(0) );
            }
            ~lexical_stream()
            {
                #if defined(BOOST_NO_STRINGSTREAM)
                stream.freeze(false);
                #endif
            }
            bool operator<<(const Source &input)
            {
                return !(stream << input).fail();
            }
            template<typename InputStreamable>
            bool operator>>(InputStreamable &output)
            {
                return !is_pointer<InputStreamable>::value &&
                       stream >> output &&
                       stream.get() ==
#if defined(__GNUC__) && (__GNUC__<3) && defined(BOOST_NO_STD_WSTRING)
// GCC 2.9x lacks std::char_traits<>::eof().
// We use BOOST_NO_STD_WSTRING to filter out STLport and libstdc++-v3
// configurations, which do provide std::char_traits<>::eof().
    
                           EOF;
#else
                           traits_type::eof();
#endif
            }

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

            bool operator>>(std::string &output)
            {
                #if defined(BOOST_NO_STRINGSTREAM)
                stream << '\0';
                #endif
                stream.str().swap(output);
                return true;
            }
            #ifndef BOOST_LCAST_NO_WCHAR_T
            bool operator>>(std::wstring &output)
            {
                stream.str().swap(output);
                return true;
            }
            #endif

#else
            bool operator>>(std::basic_string<char_type,traits_type>& output)
            {
                stream.str().swap(output);
                return true;
            }

            template<class Alloc>
            bool operator>>(std::basic_string<char_type,traits_type,Alloc>& out)
            {
                std::basic_string<char_type,traits_type> str(stream.str());
                out.assign(str.begin(), str.end());
                return true;
            }
#endif
        private:
            #if defined(BOOST_NO_STRINGSTREAM)
            std::strstream stream;
            #elif defined(BOOST_NO_STD_LOCALE)
            std::stringstream stream;
            #else
            std::basic_stringstream<char_type,traits_type> stream;
            #endif
        };
    }

    namespace detail // optimized stream wrapper
    {
        // String representation of Source has an upper limit.
        template< class CharT // a result of widest_char transformation
                , class Base // lexical_streambuf_fake or basic_streambuf<CharT>
                , class Traits // usually char_traits<CharT>
                >
        class lexical_stream_limited_src : public Base
        {
            // A string representation of Source is written to [start, finish).
            // Currently, it is assumed that [start, finish) is big enough
            // to hold a string representation of any Source value.
            CharT* start;
            CharT* finish;

        private:

            static void widen_and_assign(char*p, char ch)
            {
                Traits::assign(*p, ch);
            }

#ifndef BOOST_LCAST_NO_WCHAR_T
            static void widen_and_assign(wchar_t* p, char ch)
            {
                // TODO: use BOOST_NO_STD_LOCALE
                std::locale loc;
                wchar_t w = BOOST_USE_FACET(std::ctype<wchar_t>, loc).widen(ch);
                Traits::assign(*p, w);
            }

            static void widen_and_assign(wchar_t* p, wchar_t ch)
            {
                Traits::assign(*p, ch);
            }

            static void widen_and_assign(char*, wchar_t ch); // undefined
#endif

            template<class OutputStreamable>
            bool lcast_put(const OutputStreamable& input)
            {
                this->setp(start, finish);
                std::basic_ostream<CharT> stream(static_cast<Base*>(this));
                lcast_set_precision(stream, static_cast<OutputStreamable*>(0));
                bool const result = !(stream << input).fail();
                finish = this->pptr();
                return result;
            }

            // Undefined:
            lexical_stream_limited_src(lexical_stream_limited_src const&);
            void operator=(lexical_stream_limited_src const&);

        public:

            lexical_stream_limited_src(CharT* sta, CharT* fin)
              : start(sta)
              , finish(fin)
            {}

        public: // output

            template<class Alloc>
            bool operator<<(std::basic_string<CharT,Traits,Alloc> const& str)
            {
                start = const_cast<CharT*>(str.data());
                finish = start + str.length();
                return true;
            }

            bool operator<<(bool);
            bool operator<<(char);
            bool operator<<(unsigned char);
            bool operator<<(signed char);
#if !defined(BOOST_LCAST_NO_WCHAR_T) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
            bool operator<<(wchar_t);
#endif
            bool operator<<(unsigned char const*);
            bool operator<<(signed char const*);
            bool operator<<(CharT const*);
            bool operator<<(short);
            bool operator<<(int);
            bool operator<<(long);
            bool operator<<(unsigned short);
            bool operator<<(unsigned int);
            bool operator<<(unsigned long);
#if defined(BOOST_HAS_LONG_LONG)
            bool operator<<(boost::ulong_long_type);
            bool operator<<(boost::long_long_type );
#elif defined(BOOST_HAS_MS_INT64)
            bool operator<<(unsigned __int64);
            bool operator<<(         __int64);
#endif
            // These three operators use ostream and streambuf.
            // lcast_streambuf_for_source<T>::value is true.
            bool operator<<(float);
            bool operator<<(double);
            bool operator<<(long double);

        private:

            template <typename Type>
            bool input_operator_helper_unsigned(Type& output)
            {
                CharT const minus = lcast_char_constants<CharT>::minus;
                CharT const plus = lcast_char_constants<CharT>::plus;
                bool has_minus = false;

                /* We won`t use `start' any more, so no need in decrementing it after */
                if ( Traits::eq(minus,*start) )
                {
                    ++start;
                    has_minus = true;
                } else if ( Traits::eq( plus, *start ) )
                {
                    ++start;
                }

                bool const succeed = lcast_ret_unsigned<Traits>(output, start, finish);
#if (defined _MSC_VER)
# pragma warning( push )
// C4146: unary minus operator applied to unsigned type, result still unsigned
# pragma warning( disable : 4146 )
#elif defined( __BORLANDC__ )
# pragma option push -w-8041
#endif
                if (has_minus) output = static_cast<Type>(-output);
#if (defined _MSC_VER)
# pragma warning( pop )
#elif defined( __BORLANDC__ )
# pragma option pop
#endif
                return succeed;
            }

            template <typename Type>
            bool input_operator_helper_signed(Type& output)
            {
                CharT const minus = lcast_char_constants<CharT>::minus;
                CharT const plus = lcast_char_constants<CharT>::plus;
                typedef BOOST_DEDUCED_TYPENAME make_unsigned<Type>::type utype;
                utype out_tmp =0;
                bool has_minus = false;

                /* We won`t use `start' any more, so no need in decrementing it after */
                if ( Traits::eq(minus,*start) )
                {
                    ++start;
                    has_minus = true;
                } else if ( Traits::eq(plus, *start) )
                {
                    ++start;
                }

                bool succeed = lcast_ret_unsigned<Traits>(out_tmp, start, finish);
                if (has_minus) {
#if (defined _MSC_VER)
# pragma warning( push )
// C4146: unary minus operator applied to unsigned type, result still unsigned
# pragma warning( disable : 4146 )
#elif defined( __BORLANDC__ )
# pragma option push -w-8041
#endif
                    utype const comp_val = static_cast<utype>(-(std::numeric_limits<Type>::min)());
                    succeed = succeed && out_tmp<=comp_val;
                    output = -out_tmp;
#if (defined _MSC_VER)
# pragma warning( pop )
#elif defined( __BORLANDC__ )
# pragma option pop
#endif
                } else {
                    utype const comp_val = static_cast<utype>((std::numeric_limits<Type>::max)());
                    succeed = succeed && out_tmp<=comp_val;
                    output = out_tmp;
                }
                return succeed;
            }

        public: // input

            bool operator>>(unsigned short& output)
            {
                return input_operator_helper_unsigned(output);
            }

            bool operator>>(unsigned int& output)
            {
                return input_operator_helper_unsigned(output);
            }

            bool operator>>(unsigned long int& output)
            {
                return input_operator_helper_unsigned(output);
            }

            bool operator>>(short& output)
            {
                return input_operator_helper_signed(output);
            }

            bool operator>>(int& output)
            {
                return input_operator_helper_signed(output);
            }

            bool operator>>(long int& output)
            {
                return input_operator_helper_signed(output);
            }


#if defined(BOOST_HAS_LONG_LONG)
            bool operator>>( boost::ulong_long_type& output)
            {
                return input_operator_helper_unsigned(output);
            }

            bool operator>>(boost::long_long_type& output)
            {
                return input_operator_helper_signed(output);
            }

#elif defined(BOOST_HAS_MS_INT64)
            bool operator>>(unsigned __int64& output)
            {
                return input_operator_helper_unsigned(output);
            }

            bool operator>>(__int64& output)
            {
                return input_operator_helper_signed(output);
            }

#endif

            /*
             * case "-0" || "0" || "+0" :   output = false; return true;
             * case "1" || "+1":            output = true;  return true;
             * default:                     return false;
             */
            bool operator>>(bool& output)
            {
                CharT const zero = lcast_char_constants<CharT>::zero;
                CharT const plus = lcast_char_constants<CharT>::plus;
                CharT const minus = lcast_char_constants<CharT>::minus;

                switch(finish-start)
                {
                    case 1:
                        output = Traits::eq(start[0],  zero+1);
                        return output || Traits::eq(start[0], zero );
                    case 2:
                        if ( Traits::eq( plus, *start) )
                        {
                            ++start;
                            output = Traits::eq(start[0], zero +1);
                            return output || Traits::eq(start[0], zero );
                        } else
                        {
                            output = false;
                            return Traits::eq( minus, *start)
                                && Traits::eq( zero, start[1]);
                        }
                    default:
                        output = false; // Suppress warning about uninitalized variable
                        return false;
                }
            }


            // Generic istream-based algorithm.
            // lcast_streambuf_for_target<InputStreamable>::value is true.
            template<typename InputStreamable>
            bool operator>>(InputStreamable& output)
            {
#if (defined _MSC_VER)
# pragma warning( push )
  // conditional expression is constant
# pragma warning( disable : 4127 )
#endif
                if(is_pointer<InputStreamable>::value)
                    return false;

                this->setg(start, start, finish);
                std::basic_istream<CharT> stream(static_cast<Base*>(this));
                stream.unsetf(std::ios::skipws);
                lcast_set_precision(stream, static_cast<InputStreamable*>(0));
#if (defined _MSC_VER)
# pragma warning( pop )
#endif
                return stream >> output &&
                    stream.get() ==
#if defined(__GNUC__) && (__GNUC__<3) && defined(BOOST_NO_STD_WSTRING)
        // GCC 2.9x lacks std::char_traits<>::eof().
        // We use BOOST_NO_STD_WSTRING to filter out STLport and libstdc++-v3
        // configurations, which do provide std::char_traits<>::eof().

                    EOF;
#else
                Traits::eof();
#endif
            }

            bool operator>>(CharT&);
            bool operator>>(unsigned char&);
            bool operator>>(signed char&);

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
// This #if is in sync with lcast_streambuf_for_target

            bool operator>>(std::string&);

#ifndef BOOST_LCAST_NO_WCHAR_T
            bool operator>>(std::wstring&);
#endif

#else
            template<class Alloc>
            bool operator>>(std::basic_string<CharT,Traits,Alloc>& str)
            {
                str.assign(start, finish);
                return true;
            }
#endif
        };

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                bool value)
        {
            typedef typename Traits::int_type int_type;
            CharT const czero = lcast_char_constants<CharT>::zero;
            int_type const zero = Traits::to_int_type(czero);
            Traits::assign(*start, Traits::to_char_type(zero + value));
            finish = start + 1;
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                char ch)
        {
            widen_and_assign(start, ch);
            finish = start + 1;
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
               unsigned char ch)
        {
            return ((*this) << static_cast<char>(ch));
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
               signed char ch)
        {
            return ((*this) << static_cast<char>(ch));
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
               unsigned char const* ch)
        {
            return ((*this) << reinterpret_cast<char const*>(ch));
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
               signed char const* ch)
        {
            return ((*this) << reinterpret_cast<char const*>(ch));
        }

#if !defined(BOOST_LCAST_NO_WCHAR_T) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                wchar_t ch)
        {
            widen_and_assign(start, ch);
            finish = start + 1;
            return true;
        }
#endif

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                short n)
        {
            start = lcast_put_unsigned<Traits>(lcast_to_unsigned(n), finish);
            if(n < 0)
            {
                --start;
                CharT const minus = lcast_char_constants<CharT>::minus;
                Traits::assign(*start, minus);
            }
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                int n)
        {
            start = lcast_put_unsigned<Traits>(lcast_to_unsigned(n), finish);
            if(n < 0)
            {
                --start;
                CharT const minus = lcast_char_constants<CharT>::minus;
                Traits::assign(*start, minus);
            }
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                long n)
        {
            start = lcast_put_unsigned<Traits>(lcast_to_unsigned(n), finish);
            if(n < 0)
            {
                --start;
                CharT const minus = lcast_char_constants<CharT>::minus;
                Traits::assign(*start, minus);
            }
            return true;
        }

#if defined(BOOST_HAS_LONG_LONG)
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                boost::long_long_type n)
        {
            start = lcast_put_unsigned<Traits>(lcast_to_unsigned(n), finish);
            if(n < 0)
            {
                --start;
                CharT const minus = lcast_char_constants<CharT>::minus;
                Traits::assign(*start, minus);
            }
            return true;
        }
#elif defined(BOOST_HAS_MS_INT64)
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                __int64 n)
        {
            start = lcast_put_unsigned<Traits>(lcast_to_unsigned(n), finish);
            if(n < 0)
            {
                --start;
                CharT const minus = lcast_char_constants<CharT>::minus;
                Traits::assign(*start, minus);
            }
            return true;
        }
#endif

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                unsigned short n)
        {
            start = lcast_put_unsigned<Traits>(n, finish);
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                unsigned int n)
        {
            start = lcast_put_unsigned<Traits>(n, finish);
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                unsigned long n)
        {
            start = lcast_put_unsigned<Traits>(n, finish);
            return true;
        }

#if defined(BOOST_HAS_LONG_LONG)
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                boost::ulong_long_type n)
        {
            start = lcast_put_unsigned<Traits>(n, finish);
            return true;
        }
#elif defined(BOOST_HAS_MS_INT64)
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                unsigned __int64 n)
        {
            start = lcast_put_unsigned<Traits>(n, finish);
            return true;
        }
#endif

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                float val)
        {
            return this->lcast_put(val);
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                double val)
        {
            return this->lcast_put(val);
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                long double val)
        {
            return this->lcast_put(val);
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator<<(
                CharT const* str)
        {
            start = const_cast<CharT*>(str);
            finish = start + Traits::length(str);
            return true;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator>>(
                CharT& output)
        {
            bool const ok = (finish - start == 1);
            if(ok)
                Traits::assign(output, *start);
            return ok;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator>>(
                unsigned char& output)
        {
            BOOST_STATIC_ASSERT( sizeof(CharT) == sizeof(unsigned char) );
            bool const ok = (finish - start == 1);
            if(ok) {
                CharT out;
                Traits::assign(out, *start);
                output = static_cast<signed char>(out);
            }
            return ok;
        }

        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator>>(
                signed char& output)
        {
            BOOST_STATIC_ASSERT( sizeof(CharT) == sizeof(signed char) );
            bool const ok = (finish - start == 1);
            if(ok) {
                CharT out;
                Traits::assign(out, *start);
                output = static_cast<signed char>(out);
            }
            return ok;
        }

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator>>(
                std::string& str)
        {
            str.assign(start, finish);
            return true;
        }

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<typename CharT, class Base, class Traits>
        inline bool lexical_stream_limited_src<CharT,Base,Traits>::operator>>(
                std::wstring& str)
        {
            str.assign(start, finish);
            return true;
        }
#endif
#endif
    }

    namespace detail // lcast_streambuf_for_source
    {
        // Returns true if optimized stream wrapper needs ostream for writing.
        template<class Source>
        struct lcast_streambuf_for_source
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };

        template<>
        struct lcast_streambuf_for_source<float>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };
 
        template<>
        struct lcast_streambuf_for_source<double>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };
  
        template<>
        struct lcast_streambuf_for_source<long double>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };
    }

    namespace detail // lcast_streambuf_for_target
    {
        // Returns true if optimized stream wrapper needs istream for reading.
        template<class Target>
        struct lcast_streambuf_for_target
        {
            BOOST_STATIC_CONSTANT(bool, value =
                (
                 ::boost::type_traits::ice_not< is_integral<Target>::value >::value
                )
            );
        };

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        template<class Traits, class Alloc>
        struct lcast_streambuf_for_target<
                    std::basic_string<char,Traits,Alloc> >
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<class Traits, class Alloc>
        struct lcast_streambuf_for_target<
                    std::basic_string<wchar_t,Traits,Alloc> >
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };
#endif
#else
        template<>
        struct lcast_streambuf_for_target<std::string>
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };

#ifndef BOOST_LCAST_NO_WCHAR_T
        template<>
        struct lcast_streambuf_for_target<std::wstring>
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };
#endif
#endif
    }

    #ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

    // call-by-const reference version

    namespace detail
    {
        template<class T>
        struct array_to_pointer_decay
        {
            typedef T type;
        };

        template<class T, std::size_t N>
        struct array_to_pointer_decay<T[N]>
        {
            typedef const T * type;
        };

#if (defined _MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4701 ) // possible use of ... before initialization
# pragma warning( disable : 4702 ) // unreachable code
# pragma warning( disable : 4267 ) // conversion from 'size_t' to 'unsigned int'
#endif

        template< typename Target
                , typename Source
                , bool Unlimited // string representation of Source is unlimited
                , typename CharT
                >
        Target lexical_cast(
            BOOST_DEDUCED_TYPENAME boost::call_traits<Source>::param_type arg,
            CharT* buf, std::size_t src_len)
        {
            typedef BOOST_DEDUCED_TYPENAME
                deduce_char_traits<CharT,Target,Source>::type traits;

            typedef BOOST_DEDUCED_TYPENAME boost::mpl::if_c<
                lcast_streambuf_for_target<Target>::value ||
                lcast_streambuf_for_source<Source>::value
              , std::basic_streambuf<CharT>
              , lexical_streambuf_fake
              >::type base;

            BOOST_DEDUCED_TYPENAME boost::mpl::if_c<
                Unlimited
              , detail::lexical_stream<Target,Source,traits>
              , detail::lexical_stream_limited_src<CharT,base,traits>
              >::type interpreter(buf, buf + src_len);

            Target result;
            if(!(interpreter << arg && interpreter >> result))
              BOOST_LCAST_THROW_BAD_CAST(Source, Target);
            return result;
        }
#if (defined _MSC_VER)
# pragma warning( pop )
#endif

        template<typename T>
        struct is_stdstring
        {
            BOOST_STATIC_CONSTANT(bool, value = false );
        };

        template<typename CharT, typename Traits, typename Alloc>
        struct is_stdstring< std::basic_string<CharT, Traits, Alloc> >
        {
            BOOST_STATIC_CONSTANT(bool, value = true );
        };

        template<typename T>
        struct is_char_or_wchar
        {
#ifndef BOOST_LCAST_NO_WCHAR_T
            BOOST_STATIC_CONSTANT(bool, value =
                    (
                    ::boost::type_traits::ice_or<
                         is_same< T, char >::value,
                         is_same< T, wchar_t >::value,
                         is_same< T, unsigned char >::value,
                         is_same< T, signed char >::value
                    >::value
                    )
            );
#else
            BOOST_STATIC_CONSTANT(bool, value =
                    (
                    ::boost::type_traits::ice_or<
                         is_same< T, char >::value,
                         is_same< T, unsigned char >::value,
                         is_same< T, signed char >::value
                    >::value
                    )
            );
#endif
        };

        template<typename Target, typename Source>
        struct is_arithmetic_and_not_xchars
        {
            BOOST_STATIC_CONSTANT(bool, value =
               (
                   ::boost::type_traits::ice_and<
                           is_arithmetic<Source>::value,
                           is_arithmetic<Target>::value,
                           ::boost::type_traits::ice_not<
                                detail::is_char_or_wchar<Target>::value
                           >::value,
                           ::boost::type_traits::ice_not<
                                detail::is_char_or_wchar<Source>::value
                           >::value
                   >::value
               )
            );
        };

        /*
         * is_xchar_to_xchar<Target, Source>::value is true, when
         * Target and Souce are the same char types, or when
         * Target and Souce are char types of the same size.
         */
        template<typename Target, typename Source>
        struct is_xchar_to_xchar
        {
            BOOST_STATIC_CONSTANT(bool, value =
                (
                    ::boost::type_traits::ice_or<
                        ::boost::type_traits::ice_and<
                             is_same<Source,Target>::value,
                             is_char_or_wchar<Target>::value
                        >::value,
                        ::boost::type_traits::ice_and<
                             ::boost::type_traits::ice_eq< sizeof(char),sizeof(Target)>::value,
                             ::boost::type_traits::ice_eq< sizeof(char),sizeof(Source)>::value,
                             is_char_or_wchar<Target>::value,
                             is_char_or_wchar<Source>::value
                        >::value
                    >::value
                )
            );
        };

        template<typename Target, typename Source>
        struct is_char_array_to_stdstring
        {
            BOOST_STATIC_CONSTANT(bool, value = false );
        };

        template<typename CharT, typename Traits, typename Alloc>
        struct is_char_array_to_stdstring< std::basic_string<CharT, Traits, Alloc>, CharT* >
        {
            BOOST_STATIC_CONSTANT(bool, value = true );
        };

        template<typename CharT, typename Traits, typename Alloc>
        struct is_char_array_to_stdstring< std::basic_string<CharT, Traits, Alloc>, const CharT* >
        {
            BOOST_STATIC_CONSTANT(bool, value = true );
        };

        template<typename Target, typename Source>
        struct lexical_cast_do_cast
        {
            static inline Target lexical_cast_impl(const Source &arg)
            {
                typedef typename detail::array_to_pointer_decay<Source>::type src;

                typedef typename detail::widest_char<
                typename detail::stream_char<Target>::type
                , typename detail::stream_char<src>::type
                >::type char_type;

                typedef detail::lcast_src_length<char_type, src> lcast_src_length;
                std::size_t const src_len = lcast_src_length::value;
                char_type buf[src_len + 1];
                lcast_src_length::check_coverage();
                return detail::lexical_cast<Target, src, !src_len>(arg, buf, src_len);
            }
        };

        template<typename Source>
        struct lexical_cast_copy
        {
            static inline Source lexical_cast_impl(const Source &arg)
            {
                return arg;
            }
        };

        class precision_loss_error : public boost::numeric::bad_numeric_cast
        {
         public:
            virtual const char * what() const throw()
             {  return "bad numeric conversion: precision loss error"; }
        };

        template<class S >
        struct throw_on_precision_loss
        {
         typedef boost::numeric::Trunc<S> Rounder;
         typedef S source_type ;

         typedef typename mpl::if_< is_arithmetic<S>,S,S const&>::type argument_type ;

         static source_type nearbyint ( argument_type s )
         {
            source_type orig_div_round = s / Rounder::nearbyint(s);

            if ( (orig_div_round > 1 ? orig_div_round - 1 : 1 - orig_div_round) > std::numeric_limits<source_type>::epsilon() )
               BOOST_THROW_EXCEPTION( precision_loss_error() );
            return s ;
         }

         typedef typename Rounder::round_style round_style;
        } ;

        template<typename Target, typename Source>
        struct lexical_cast_dynamic_num_not_ignoring_minus
        {
            static inline Target lexical_cast_impl(const Source &arg)
            {
                try{
                    typedef boost::numeric::converter<
                            Target,
                            Source,
                            boost::numeric::conversion_traits<Target,Source>,
                            boost::numeric::def_overflow_handler,
                            throw_on_precision_loss<Source>
                    > Converter ;

                    return Converter::convert(arg);
                } catch( ::boost::numeric::bad_numeric_cast const& ) {
                    BOOST_LCAST_THROW_BAD_CAST(Source, Target);
                }
            }
        };

        template<typename Target, typename Source>
        struct lexical_cast_dynamic_num_ignoring_minus
        {
            static inline Target lexical_cast_impl(const Source &arg)
            {
                try{
                    typedef boost::numeric::converter<
                            Target,
                            Source,
                            boost::numeric::conversion_traits<Target,Source>,
                            boost::numeric::def_overflow_handler,
                            throw_on_precision_loss<Source>
                    > Converter ;

                    bool has_minus = ( arg < 0);
                    if ( has_minus ) {
                        return static_cast<Target>(-Converter::convert(-arg));
                    } else {
                        return Converter::convert(arg);
                    }
                } catch( ::boost::numeric::bad_numeric_cast const& ) {
                    BOOST_LCAST_THROW_BAD_CAST(Source, Target);
                }
            }
        };

        /*
         * lexical_cast_dynamic_num follows the rules:
         * 1) If Source can be converted to Target without precision loss and
         * without overflows, then assign Source to Target and return
         *
         * 2) If Source is less than 0 and Target is an unsigned integer,
         * then negate Source, check the requirements of rule 1) and if
         * successful, assign static_casted Source to Target and return
         *
         * 3) Otherwise throw a bad_lexical_cast exception
         *
         *
         * Rule 2) required because boost::lexical_cast has the behavior of
         * stringstream, which uses the rules of scanf for conversions. And
         * in the C99 standard for unsigned input value minus sign is
         * optional, so if a negative number is read, no errors will arise
         * and the result will be the two's complement.
         */
        template<typename Target, typename Source>
        struct lexical_cast_dynamic_num
        {
            static inline Target lexical_cast_impl(const Source &arg)
            {
                typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::if_c<
                    ::boost::type_traits::ice_and<
                        ::boost::type_traits::ice_or<
                            ::boost::is_signed<Source>::value,
                            ::boost::is_float<Source>::value
                        >::value,
                        ::boost::type_traits::ice_not<
                            is_same<Source, bool>::value
                        >::value,
                        ::boost::type_traits::ice_not<
                            is_same<Target, bool>::value
                        >::value,
                        ::boost::is_unsigned<Target>::value
                    >::value,
                    lexical_cast_dynamic_num_ignoring_minus<Target, Source>,
                    lexical_cast_dynamic_num_not_ignoring_minus<Target, Source>
                >::type caster_type;

                return caster_type::lexical_cast_impl(arg);
            }
        };
    }

    template<typename Target, typename Source>
    inline Target lexical_cast(const Source &arg)
    {
        typedef BOOST_DEDUCED_TYPENAME detail::array_to_pointer_decay<Source>::type src;

        typedef BOOST_DEDUCED_TYPENAME ::boost::type_traits::ice_or<
                detail::is_xchar_to_xchar<Target, src>::value,
                detail::is_char_array_to_stdstring<Target,src>::value,
                ::boost::type_traits::ice_and<
                     is_same<Target, src>::value,
                     detail::is_stdstring<Target>::value
                >::value
        > do_copy_type;

        typedef BOOST_DEDUCED_TYPENAME
                detail::is_arithmetic_and_not_xchars<Target, src> do_copy_with_dynamic_check_type;

        typedef BOOST_DEDUCED_TYPENAME ::boost::mpl::if_c<
            do_copy_type::value,
            detail::lexical_cast_copy<src>,
            BOOST_DEDUCED_TYPENAME ::boost::mpl::if_c<
                 do_copy_with_dynamic_check_type::value,
                 detail::lexical_cast_dynamic_num<Target, src>,
                 detail::lexical_cast_do_cast<Target, src>
            >::type
        >::type caster_type;

        return caster_type::lexical_cast_impl(arg);
    }

    #else

    // call-by-value fallback version (deprecated)

    template<typename Target, typename Source>
    Target lexical_cast(Source arg)
    {
        typedef typename detail::widest_char< 
            BOOST_DEDUCED_TYPENAME detail::stream_char<Target>::type 
          , BOOST_DEDUCED_TYPENAME detail::stream_char<Source>::type 
        >::type char_type; 

        typedef std::char_traits<char_type> traits;
        detail::lexical_stream<Target, Source, traits> interpreter;
        Target result;

        if(!(interpreter << arg && interpreter >> result))
          BOOST_LCAST_THROW_BAD_CAST(Source, Target);
        return result;
    }

    #endif
}

// Copyright Kevlin Henney, 2000-2005.
// Copyright Alexander Nasonov, 2006-2010.
// Copyright Antony Polukhin, 2011.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#undef BOOST_LCAST_NO_WCHAR_T
#endif
