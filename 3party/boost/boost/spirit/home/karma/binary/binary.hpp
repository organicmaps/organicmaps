//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_BINARY_MAY_04_2007_0904AM)
#define BOOST_SPIRIT_KARMA_BINARY_MAY_04_2007_0904AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/detail/endian.hpp>

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_ENABLE_BINARY(name)                                      \
    template <>                                                               \
    struct use_terminal<karma::domain, tag::name>                             \
      : mpl::true_ {};                                                        \
                                                                              \
    template <typename A0>                                                    \
    struct use_terminal<karma::domain                                         \
        , terminal_ex<tag::name, fusion::vector1<A0> > >                      \
      : mpl::or_<is_integral<A0>, is_enum<A0> > {};                           \
                                                                              \
    template <>                                                               \
    struct use_lazy_terminal<karma::domain, tag::name, 1> : mpl::true_ {};    \
                                                                              \
/***/

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    BOOST_SPIRIT_ENABLE_BINARY(byte_)                   // enables byte_
    BOOST_SPIRIT_ENABLE_BINARY(word)                    // enables word
    BOOST_SPIRIT_ENABLE_BINARY(big_word)                // enables big_word
    BOOST_SPIRIT_ENABLE_BINARY(little_word)             // enables little_word
    BOOST_SPIRIT_ENABLE_BINARY(dword)                   // enables dword
    BOOST_SPIRIT_ENABLE_BINARY(big_dword)               // enables big_dword
    BOOST_SPIRIT_ENABLE_BINARY(little_dword)            // enables little_dword
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_ENABLE_BINARY(qword)                   // enables qword
    BOOST_SPIRIT_ENABLE_BINARY(big_qword)               // enables big_qword
    BOOST_SPIRIT_ENABLE_BINARY(little_qword)            // enables little_qword
#endif

}}

#undef BOOST_SPIRIT_ENABLE_BINARY

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using boost::spirit::byte_;
    using boost::spirit::word;
    using boost::spirit::big_word;
    using boost::spirit::little_word;
    using boost::spirit::dword;
    using boost::spirit::big_dword;
    using boost::spirit::little_dword;
#ifdef BOOST_HAS_LONG_LONG
    using boost::spirit::qword;
    using boost::spirit::big_qword;
    using boost::spirit::little_qword;
#endif
#endif

    using boost::spirit::byte_type;
    using boost::spirit::word_type;
    using boost::spirit::big_word_type;
    using boost::spirit::little_word_type;
    using boost::spirit::dword_type;
    using boost::spirit::big_dword_type;
    using boost::spirit::little_dword_type;
#ifdef BOOST_HAS_LONG_LONG
    using boost::spirit::qword_type;
    using boost::spirit::big_qword_type;
    using boost::spirit::little_qword_type;
#endif

    namespace detail
    {
        template <int bits>
        struct integer
        {
#ifdef BOOST_HAS_LONG_LONG
            BOOST_SPIRIT_ASSERT_MSG(
                bits == 8 || bits == 16 || bits == 32 || bits == 64,
                not_supported_binary_size, ());
#else
            BOOST_SPIRIT_ASSERT_MSG(
                bits == 8 || bits == 16 || bits == 32,
                not_supported_binary_size, ());
#endif
        };

        template <>
        struct integer<8>
        {
            typedef uint_least8_t type;
        };

        template <>
        struct integer<16>
        {
            typedef uint_least16_t type;
        };

        template <>
        struct integer<32>
        {
            typedef uint_least32_t type;
        };

#ifdef BOOST_HAS_LONG_LONG
        template <>
        struct integer<64>
        {
            typedef uint_least64_t type;
        };
#endif

        ///////////////////////////////////////////////////////////////////////
        template <BOOST_SCOPED_ENUM(boost::integer::endianness) bits>
        struct what;

        template <>
        struct what<boost::integer::endianness::native>
        {
            static info is()
            {
                return info("native-endian binary");
            }
        };

        template <>
        struct what<boost::integer::endianness::little>
        {
            static info is()
            {
                return info("little-endian binary");
            }
        };

        template <>
        struct what<boost::integer::endianness::big>
        {
            static info is()
            {
                return info("big-endian binary");
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <BOOST_SCOPED_ENUM(boost::integer::endianness) endian, int bits>
    struct any_binary_generator
      : primitive_generator<any_binary_generator<endian, bits> >
    {
        template <typename Context, typename Unused = unused_type>
        struct attribute
          : karma::detail::integer<bits>
        {};

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        static bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr)
        {
            if (!traits::has_optional_value(attr))
                return false;

            // Even if the endian types are not pod's (at least not in the
            // definition of C++03) it seems to be safe to assume they are.
            // This allows us to treat them as a sequence of consecutive bytes.
            boost::integer::endian<
                endian, typename karma::detail::integer<bits>::type, bits
            > p;

#if defined(BOOST_MSVC)
// warning C4244: 'argument' : conversion from 'const int' to 'foo', possible loss of data
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
            typedef typename karma::detail::integer<bits>::type attribute_type;
            p = traits::extract_from<attribute_type>(attr, context);
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

            unsigned char const* bytes =
                reinterpret_cast<unsigned char const*>(&p);

            for (unsigned int i = 0; i < sizeof(p); ++i)
            {
                if (!detail::generate_to(sink, *bytes++))
                    return false;
            }
            return karma::delimit_out(sink, d);     // always do post-delimiting
        }

        // this any_byte_director has no parameter attached, it needs to have
        // been initialized from a direct literal
        template <
            typename OutputIterator, typename Context, typename Delimiter>
        static bool generate(OutputIterator&, Context&, Delimiter const&
          , unused_type)
        {
            // It is not possible (doesn't make sense) to use binary generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_MSG(false,
                binary_generator_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return karma::detail::what<endian>::is();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <BOOST_SCOPED_ENUM(boost::integer::endianness) endian, int bits>
    struct literal_binary_generator
      : primitive_generator<literal_binary_generator<endian, bits> >
    {
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        template <typename T>
        literal_binary_generator(T const& t)
        {
#if defined(BOOST_MSVC)
// warning C4244: 'argument' : conversion from 'const int' to 'foo', possible loss of data
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
            data_ = t;
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
        }

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , Attribute const&) const
        {
            // Even if the endian types are not pod's (at least not in the
            // definition of C++03) it seems to be safe to assume they are
            // (but in C++0x the endian types _are_ PODs).
            // This allows us to treat them as a sequence of consecutive bytes.
            unsigned char const* bytes =
                reinterpret_cast<unsigned char const*>(&data_);

            for (unsigned int i = 0; i < sizeof(data_type); ++i)
            {
                if (!detail::generate_to(sink, *bytes++))
                    return false;
            }
            return karma::delimit_out(sink, d);  // always do post-delimiting
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return karma::detail::what<endian>::is();
        }

        typedef boost::integer::endian<
            endian, typename karma::detail::integer<bits>::type, bits
        > data_type;

        data_type data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <BOOST_SCOPED_ENUM(boost::integer::endianness) endian
          , int bits>
        struct basic_binary
        {
            typedef any_binary_generator<endian, bits> result_type;

            result_type operator()(unused_type, unused_type) const
            {
                return result_type();
            }
        };

        template <typename Modifiers
          , BOOST_SCOPED_ENUM(boost::integer::endianness) endian, int bits>
        struct basic_binary_literal
        {
            typedef literal_binary_generator<endian, bits> result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                return result_type(fusion::at_c<0>(term.args));
            }
        };
    }

#define BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(name, endian, bits)                \
    template <typename Modifiers>                                             \
    struct make_primitive<tag::name, Modifiers>                               \
      : detail::basic_binary<boost::integer::endianness::endian, bits> {};    \
                                                                              \
    template <typename Modifiers, typename A0>                                \
    struct make_primitive<terminal_ex<tag::name, fusion::vector1<A0> >        \
          , Modifiers>                                                        \
      : detail::basic_binary_literal<Modifiers                                \
        , boost::integer::endianness::endian, bits> {};                       \
                                                                              \
    /***/

    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(byte_, native, 8)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(word, native, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_word, big, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_word, little, 16)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(dword, native, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_dword, big, 32)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_dword, little, 32)
#ifdef BOOST_HAS_LONG_LONG
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(qword, native, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(big_qword, big, 64)
    BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE(little_qword, little, 64)
#endif

#undef BOOST_SPIRIT_MAKE_BINARY_PRIMITIVE

}}}

#endif
