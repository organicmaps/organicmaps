//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_CHAR_TOKEN_DEF_MAR_28_2007_0626PM)
#define BOOST_SPIRIT_LEX_CHAR_TOKEN_DEF_MAR_28_2007_0626PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/lex/domain.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/meta_compiler.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<lex::domain, char>               // enables 'x'
      : mpl::true_ {};

    template <>
    struct use_terminal<lex::domain, char[2]>            // enables "x"
      : mpl::true_ {};

    template <>
    struct use_terminal<lex::domain, wchar_t>            // enables wchar_t
      : mpl::true_ {};

    template <>
    struct use_terminal<lex::domain, wchar_t[2]>         // enables L"x"
      : mpl::true_ {};

    template <typename CharEncoding, typename A0>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::char_, CharEncoding>     // enables char_('x'), char_("x")
          , fusion::vector1<A0>
        >
    > : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace lex
{ 
    using spirit::lit;                    // lit('x') is equivalent to 'x'

    // use char_ from standard character set by default
    using spirit::standard::char_type;
    using spirit::standard::char_;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  char_token_def 
    //      represents a single character token definition
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharEncoding = char_encoding::standard>
    struct char_token_def
      : primitive_lexer<char_token_def<CharEncoding> >
    {
        typedef typename CharEncoding::char_type char_type;

        char_token_def(char_type ch) 
          : ch(ch), unique_id_(std::size_t(~0)) {}

        template <typename LexerDef, typename String>
        void collect(LexerDef& lexdef, String const& state) const
        {
            unique_id_ = lexdef.add_token (state.c_str(), ch
              , static_cast<std::size_t>(ch));
        }

        template <typename LexerDef>
        void add_actions(LexerDef&) const {}

        std::size_t id() const { return static_cast<std::size_t>(ch); }
        std::size_t unique_id() const { return unique_id_; }

        char_type ch;
        mutable std::size_t unique_id_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Lexer generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename CharEncoding>
        struct basic_literal
        {
            typedef char_token_def<CharEncoding> result_type;

            template <typename Char>
            result_type operator()(Char ch, unused_type) const
            {
                return result_type(ch);
            }

            template <typename Char>
            result_type operator()(Char const* str, unused_type) const
            {
                return result_type(str[0]);
            }
        };
    }

    // literals: 'x', "x"
    template <typename Modifiers>
    struct make_primitive<char, Modifiers>
      : detail::basic_literal<char_encoding::standard> {};

    template <typename Modifiers>
    struct make_primitive<char const(&)[2], Modifiers>
      : detail::basic_literal<char_encoding::standard> {};

    // literals: L'x', L"x"
    template <typename Modifiers>
    struct make_primitive<wchar_t, Modifiers>
      : detail::basic_literal<char_encoding::standard_wide> {};

    template <typename Modifiers>
    struct make_primitive<wchar_t const(&)[2], Modifiers>
      : detail::basic_literal<char_encoding::standard_wide> {};

    // handle char_('x')
    template <typename CharEncoding, typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<A0>
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    // handle char_("x")
    template <typename CharEncoding, typename Modifiers, typename Char>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::char_, CharEncoding>
          , fusion::vector1<Char(&)[2]>   // single char strings
        >
      , Modifiers>
    {
        typedef char_token_def<CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args)[0]);
        }
    };

}}}  // namespace boost::spirit::lex

#endif 
