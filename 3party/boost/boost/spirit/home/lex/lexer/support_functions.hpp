//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_LEX_SUPPORT_FUNCTIONS_JUN_08_2009_0211PM)
#define SPIRIT_LEX_SUPPORT_FUNCTIONS_JUN_08_2009_0211PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/argument.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/value.hpp>
#include <boost/spirit/home/phoenix/core/as_actor.hpp>
#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>
#include <boost/spirit/home/lex/lexer/pass_flags.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    // The function less() is used by the implementation of the support
    // function lex::less(). Its functionality is equivalent to flex' function 
    // yyless(): it returns an iterator positioned to the nth input character 
    // beyond the current start iterator (i.e. by assigning the return value to 
    // the placeholder '_end' it is possible to return all but the first n 
    // characters of the current token back to the input stream. 
    //
    //  This Phoenix actor is invoked whenever the function lex::less(n) is 
    //  used inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ _end = lex::less(4) ];
    //
    //  The example shows how to limit the length of the matched identifier to 
    //  four characters.
    //
    //  Note: the function lex::less() has no effect if used on it's own, you 
    //        need to use the returned result in order to make use of its 
    //        functionality.
    template <typename Actor>
    struct less_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename
                remove_const<
                    typename mpl::at_c<typename Env::args_type, 4>::type
                >::type
            context_type;
            typedef typename context_type::base_iterator_type type;
        };

        template <typename Env>
        typename result<Env>::type 
        eval(Env const& env) const
        {
            typename result<Env>::type it;
            return fusion::at_c<4>(env.args()).less(it, actor_());
        }

        less_type(Actor const& actor)
          : actor_(actor) {}

        Actor actor_;
    };

    //  The function lex::less() is used to create a Phoenix actor allowing to
    //  implement functionality similar to flex' function yyless(). 
    template <typename T>
    inline phoenix::actor<less_type<typename phoenix::as_actor<T>::type> >
    less(T const& v)
    {
        typedef typename phoenix::as_actor<T>::type actor_type;
        return less_type<actor_type>(phoenix::as_actor<T>::convert(v));
    }

    ///////////////////////////////////////////////////////////////////////////
    // The function more() is used by the implemention of the support function 
    // lex::more(). Its functionality is equivalent to flex' function yymore(): 
    // it tells the lexer that the next time it matches a rule, the 
    // corresponding token should be appended onto the current token value 
    // rather than replacing it.
    //
    //  This Phoenix actor is invoked whenever the function lex::less(n) is 
    //  used inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ lex::more() ];
    //
    //  The example shows how prefix the next matched token with the matched
    //  identifier.
    struct more_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef void type;
        };

        template <typename Env>
        void eval(Env const& env) const
        {
            fusion::at_c<4>(env.args()).more();
        }
    };

    //  The function lex::more() is used to create a Phoenix actor allowing to
    //  implement functionality similar to flex' function yymore(). 
    inline phoenix::actor<more_type>
    more()
    {
        return more_type();
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Actor>
    struct lookahead_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef bool type;
        };

        template <typename Env>
        bool eval(Env const& env) const
        {
            return fusion::at_c<4>(env.args()).lookahead(actor_());
        }

        lookahead_type(Actor const& actor)
          : actor_(actor) {}

        Actor actor_;
    };

    template <typename T>
    inline phoenix::actor<
        lookahead_type<typename phoenix::as_actor<T>::type> >
    lookahead(T const& id)
    {
        typedef typename phoenix::as_actor<T>::type actor_type;
        return lookahead_type<actor_type>(phoenix::as_actor<T>::convert(id));
    }

    template <typename Attribute, typename Char, typename Idtype>
    inline phoenix::actor<
        lookahead_type<typename phoenix::as_actor<Idtype>::type> >
    lookahead(token_def<Attribute, Char, Idtype> const& tok)
    {
        typedef typename phoenix::as_actor<Idtype>::type actor_type;
        return lookahead_type<actor_type>(
            phoenix::as_actor<Idtype>::convert(tok.id()));
    }

    ///////////////////////////////////////////////////////////////////////////
    inline BOOST_SCOPED_ENUM(pass_flags) ignore()
    {
        return pass_flags::pass_ignore;
    }

}}}

#endif
