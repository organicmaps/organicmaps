//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_STRING_TOKEN_DEF_MAR_28_2007_0722PM)
#define BOOST_SPIRIT_LEX_STRING_TOKEN_DEF_MAR_28_2007_0722PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/lex/domain.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/meta_compiler.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct use_terminal<lex::domain, T
      , typename enable_if<traits::is_string<T> >::type> // enables strings
      : mpl::true_ {};

    template <typename CharEncoding, typename A0>
    struct use_terminal<lex::domain
      , terminal_ex<
            tag::char_code<tag::string, CharEncoding>   // enables string(str)
          , fusion::vector1<A0> >
    > : traits::is_string<A0> {};

}}

namespace boost { namespace spirit { namespace lex
{ 
    // use string from standard character set by default
    using spirit::standard::string_type;
    using spirit::standard::string;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  string_token_def 
    //      represents a string based token definition
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename String, typename CharEncoding = char_encoding::standard>
    struct string_token_def
      : primitive_lexer<string_token_def<String, CharEncoding> >
    {
        typedef typename
            remove_const<typename traits::char_type_of<String>::type>::type
        char_type;
        typedef std::basic_string<char_type> string_type;

        string_token_def(typename add_reference<String>::type str)
          : str_(str), id_(std::size_t(~0)) {}

        template <typename LexerDef, typename State>
        void collect(LexerDef& lexdef, State const& state) const
        {
            typedef typename LexerDef::id_type id_type;
            if (std::size_t(~0) == id_)
                id_ = lexdef.get_next_id();
            unique_id_ = lexdef.add_token (state.c_str(), str_, id_);
        }

        template <typename LexerDef>
        void add_actions(LexerDef&) const {}

        std::size_t id() const { return id_; }
        std::size_t unique_id() const { return unique_id_; }

        string_type str_;
        mutable std::size_t id_;
        mutable std::size_t unique_id_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Lex generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Modifiers>
    struct make_primitive<T, Modifiers
      , typename enable_if<traits::is_string<T> >::type>
    {
        typedef typename add_const<T>::type const_string;
        typedef string_token_def<const_string> result_type;

        result_type operator()(
            typename add_reference<const_string>::type str, unused_type) const
        {
            return result_type(str);
        }
    };

    template <typename Modifiers, typename CharEncoding, typename A0>
    struct make_primitive<
        terminal_ex<
            tag::char_code<tag::string, CharEncoding>
          , fusion::vector1<A0> >
      , Modifiers>
    {
        typedef typename add_const<A0>::type const_string;
        typedef string_token_def<const_string, CharEncoding> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

}}}  // namespace boost::spirit::lex

#endif 
