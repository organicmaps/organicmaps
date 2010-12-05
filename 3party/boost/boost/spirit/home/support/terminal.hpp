/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TERMINAL_NOVEMBER_04_2008_0906AM)
#define BOOST_SPIRIT_TERMINAL_NOVEMBER_04_2008_0906AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/proto/proto.hpp>
#include <boost/fusion/include/unused.hpp>
#include <boost/fusion/include/void.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/home/support/meta_compiler.hpp>
#include <boost/spirit/home/support/detail/make_vector.hpp>

namespace boost { namespace spirit
{
    template <typename Terminal, typename Args>
    struct terminal_ex
    {
        typedef Terminal terminal_type;
        typedef Args args_type;

        terminal_ex(Args const& args)
          : args(args) {}
        terminal_ex(Args const& args, Terminal const& term)
          : args(args), term(term) {}

        Args args;  // Args is guaranteed to be a fusion::vectorN so you
                    // can use that template for detection and specialization
        Terminal term;
    };

    template <typename Terminal, typename Actor, int Arity>
    struct lazy_terminal
    {
        typedef Terminal terminal_type;
        typedef Actor actor_type;
        static int const arity = Arity;

        lazy_terminal(Actor const& actor)
          : actor(actor) {}
        lazy_terminal(Actor const& actor, Terminal const& term)
          : actor(actor), term(term) {}

        Actor actor;
        Terminal term;
    };

    template <typename Domain, typename Terminal, int Arity, typename Enable = void>
    struct use_lazy_terminal : mpl::false_ {};

    template <typename Domain, typename Terminal, int Arity, typename Enable = void>
    struct use_lazy_directive : mpl::false_ {};

    template <typename Domain, typename Terminal, int Arity, typename Actor>
    struct use_terminal<Domain, lazy_terminal<Terminal, Actor, Arity> >
        : use_lazy_terminal<Domain, Terminal, Arity> {};

    template <typename Domain, typename Terminal, int Arity, typename Actor>
    struct use_directive<Domain, lazy_terminal<Terminal, Actor, Arity> >
        : use_lazy_directive<Domain, Terminal, Arity> {};

    template <
        typename F
      , typename A0 = unused_type
      , typename A1 = unused_type
      , typename A2 = unused_type
      , typename Unused = unused_type
    >
    struct make_lazy;

    template <typename F, typename A0>
    struct make_lazy<F, A0>
    {
        typedef typename
            proto::terminal<
                lazy_terminal<
                    typename F::terminal_type
                  , phoenix::actor<
                        typename phoenix::as_composite<
                            phoenix::detail::function_eval<1>, F, A0
                        >::type
                    >
                  , 1 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::compose<phoenix::detail::function_eval<1> >(f, _0)
              , f.proto_base().child0
            ));
        }
    };

    template <typename F, typename A0, typename A1>
    struct make_lazy<F, A0, A1>
    {
        typedef typename
            proto::terminal<
               lazy_terminal<
                    typename F::terminal_type
                  , phoenix::actor<
                        typename phoenix::as_composite<
                            phoenix::detail::function_eval<2>, F, A0, A1
                        >::type
                    >
                  , 2 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0, A1 const& _1) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::compose<phoenix::detail::function_eval<2> >(f, _0, _1)
              , f.proto_base().child0
            ));
        }
    };

    template <typename F, typename A0, typename A1, typename A2>
    struct make_lazy<F, A0, A1, A2>
    {
        typedef typename
            proto::terminal<
               lazy_terminal<
                    typename F::terminal_type
                  , phoenix::actor<
                        typename phoenix::as_composite<
                            phoenix::detail::function_eval<3>, F, A0, A1, A2
                        >::type
                    >
                  , 3 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0, A1 const& _1, A2 const& _2) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::compose<phoenix::detail::function_eval<3> >(f, _0, _1, _2)
              , f.proto_base().child0
            ));
        }
    };

    namespace detail
    {
        // Helper struct for SFINAE purposes
        template <bool C>
        struct bool_;
        template <>
        struct bool_<true> : mpl::bool_<true>
          { typedef bool_<true>* is_true; };
        template <>
        struct bool_<false> : mpl::bool_<false>
          { typedef bool_<false>* is_false; };

        // Metafunction to detect if at least one arg is a Phoenix actor
        template <
            typename A0
          , typename A1 = unused_type
          , typename A2 = unused_type
        >
        struct contains_actor
            : bool_<
                phoenix::is_actor<A0>::value
             || phoenix::is_actor<A1>::value
             || phoenix::is_actor<A2>::value
            >
        {};

        // to_lazy_arg: convert a terminal arg type to the type make_lazy needs
        template <typename A>
        struct to_lazy_arg
          : phoenix::as_actor<A> // wrap A in a Phoenix actor if not already one
        {};

        template <typename A>
        struct to_lazy_arg<const A>
          : to_lazy_arg<A>
        {};

        template <>
        struct to_lazy_arg<unused_type>
        {
            // unused arg: make_lazy wants unused_type
            typedef unused_type type;
        };

        // to_nonlazy_arg: convert a terminal arg type to the type make_vector needs
        template <typename A>
        struct to_nonlazy_arg
        {
            // identity
            typedef A type;
        };

        template <typename A>
        struct to_nonlazy_arg<const A>
          : to_nonlazy_arg<A>
        {};

        template <>
        struct to_nonlazy_arg<unused_type>
        {
            // unused arg: make_vector wants fusion::void_
            typedef fusion::void_ type;
        };
    }

    template <typename Terminal>
    struct terminal
      : proto::extends<
            typename proto::terminal<Terminal>::type
          , terminal<Terminal>
        >
    {
        typedef terminal<Terminal> this_type;
        typedef Terminal terminal_type;

        typedef proto::extends<
            typename proto::terminal<Terminal>::type
          , terminal<Terminal>
        > base_type;

        terminal() {}

        terminal(Terminal const& t)
          : base_type(proto::terminal<Terminal>::type::make(t)) {}

        template <
            bool Lazy
          , typename A0
          , typename A1
          , typename A2
        >
        struct result_helper;

        template <
            typename A0
          , typename A1
          , typename A2
        >
        struct result_helper<false, A0, A1, A2>
        {
            typedef typename
                proto::terminal<
                    terminal_ex<
                        Terminal
                      , typename detail::result_of::make_vector<
                            typename detail::to_nonlazy_arg<A0>::type
                          , typename detail::to_nonlazy_arg<A1>::type
                          , typename detail::to_nonlazy_arg<A2>::type>::type>
                >::type
            type;
        };

        template <
            typename A0
          , typename A1
          , typename A2
        >
        struct result_helper<true, A0, A1, A2>
        {
            typedef typename
                make_lazy<this_type
                  , typename detail::to_lazy_arg<A0>::type
                  , typename detail::to_lazy_arg<A1>::type
                  , typename detail::to_lazy_arg<A2>::type>::type
            type;
        };

        // FIXME: we need to change this to conform to the result_of protocol
        template <
            typename A0
          , typename A1 = unused_type
          , typename A2 = unused_type      // Support up to 3 args
        >
        struct result
        {
            typedef typename
                result_helper<
                    detail::contains_actor<A0, A1, A2>::value
                  , A0, A1, A2
                >::type
            type;
        };

        // Note: in the following overloads, SFINAE cannot
        // be done on return type because of gcc bug #24915:
        //   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=24915
        // Hence an additional, fake argument is used for SFINAE,
        // using a type which can never be a real argument type.

        // Non-lazy overloads. Only enabled when all
        // args are immediates (no Phoenix actor).

        template <typename A0>
        typename result<A0>::type
        operator()(A0 const& _0
          , typename detail::contains_actor<A0>::is_false = 0) const
        {
            typedef typename result<A0>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0)
                  , this->proto_base().child0)
            );
        }

        template <typename A0, typename A1>
        typename result<A0, A1>::type
        operator()(A0 const& _0, A1 const& _1
          , typename detail::contains_actor<A0, A1>::is_false = 0) const
        {
            typedef typename result<A0, A1>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0, _1)
                  , this->proto_base().child0)
            );
        }

        template <typename A0, typename A1, typename A2>
        typename result<A0, A1, A2>::type
        operator()(A0 const& _0, A1 const& _1, A2 const& _2
          , typename detail::contains_actor<A0, A1, A2>::is_false = 0) const
        {
            typedef typename result<A0, A1, A2>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0, _1, _2)
                  , this->proto_base().child0)
            );
        }

        // Lazy overloads. Enabled when at
        // least one arg is a Phoenix actor.

        template <typename A0>
        typename result<A0>::type
        operator()(A0 const& _0
          , typename detail::contains_actor<A0>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0));
        }

        template <typename A0, typename A1>
        typename result<A0, A1>::type
        operator()(A0 const& _0, A1 const& _1
          , typename detail::contains_actor<A0, A1>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type
              , typename phoenix::as_actor<A1>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0)
              , phoenix::as_actor<A1>::convert(_1));
        }

        template <typename A0, typename A1, typename A2>
        typename result<A0, A1, A2>::type
        operator()(A0 const& _0, A1 const& _1, A2 const& _2
          , typename detail::contains_actor<A0, A1, A2>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type
              , typename phoenix::as_actor<A1>::type
              , typename phoenix::as_actor<A2>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0)
              , phoenix::as_actor<A1>::convert(_1)
              , phoenix::as_actor<A2>::convert(_2));
        }

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        terminal& operator= (terminal const&);
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace result_of
    {
        // Calculate the type of the compound terminal if generated by one of 
        // the spirit::terminal::operator() overloads above

        // The terminal type itself is passed through without modification
        template <typename Tag>
        struct terminal
        {
            typedef spirit::terminal<Tag> type;
        };

        template <typename Tag, typename A0>
        struct terminal<Tag(A0)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0>::type type;
        };

        template <typename Tag, typename A0, typename A1>
        struct terminal<Tag(A0, A1)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0, A1>::type type;
        };

        template <typename Tag, typename A0, typename A1, typename A2>
        struct terminal<Tag(A0, A1, A2)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0, A1, A2>::type type;
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // support for stateful tag types
    namespace tag
    {
        template <typename Data, typename Tag
          , typename DataTag1 = unused_type, typename DataTag2 = unused_type>
        struct stateful_tag 
        {
            typedef Data data_type;

            stateful_tag() {}
            stateful_tag(data_type const& data) : data_(data) {}

            data_type data_;

        private:
            // silence MSVC warning C4512: assignment operator could not be generated
            stateful_tag& operator= (stateful_tag const&);
        };
    }

    template <typename Data, typename Tag
      , typename DataTag1 = unused_type, typename DataTag2 = unused_type>
    struct stateful_tag_type
      : spirit::terminal<tag::stateful_tag<Data, Tag, DataTag1, DataTag2> > 
    {
        typedef tag::stateful_tag<Data, Tag, DataTag1, DataTag2> tag_type;

        stateful_tag_type() {}
        stateful_tag_type(Data const& data)
          : spirit::terminal<tag_type>(data) {}

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        stateful_tag_type& operator= (stateful_tag_type const&);
    };

    namespace detail
    {
        // extract expression if this is a Tag
        template <typename StatefulTag>
        struct get_stateful_data
        {
            typedef typename StatefulTag::data_type data_type;

            // is invoked if given tag is != Tag
            template <typename Tag_>
            static data_type call(Tag_) { return data_type(); }

            // this is invoked if given tag is same as'Tag'
            static data_type const& call(StatefulTag const& t) { return t.data_; }
        };
    }

}}

// Define a spirit terminal. This macro may be placed in any namespace.
// Common placeholders are placed in the main boost::spirit namespace
// (see common_terminals.hpp)

#define BOOST_SPIRIT_TERMINAL(name)                                             \
    namespace tag { struct name {};  }                                          \
    typedef boost::proto::terminal<tag::name>::type name##_type;                \
    name##_type const name = {{}};                                              \
    inline void silence_unused_warnings__##name() { (void) name; }              \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_A(r, _, name)                             \
    BOOST_SPIRIT_TERMINAL(name)                                                 \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS(seq)                                      \
    BOOST_PP_SEQ_FOR_EACH(BOOST_SPIRIT_DEFINE_TERMINALS_A, _, seq)              \
    /***/

// Define a spirit extended terminal. This macro may be placed in any namespace.
// Common placeholders are placed in the main boost::spirit namespace
// (see common_terminals.hpp)

#define BOOST_SPIRIT_TERMINAL_EX(name)                                          \
    namespace tag { struct name {}; }                                           \
    typedef boost::spirit::terminal<tag::name> name##_type;                     \
    name##_type const name = name##_type();                                     \
    inline void silence_unused_warnings__##name() { (void) name; }              \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_EX_A(r, _, name)                          \
    BOOST_SPIRIT_TERMINAL_EX(name)                                              \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_EX(seq)                                   \
    BOOST_PP_SEQ_FOR_EACH(BOOST_SPIRIT_DEFINE_TERMINALS_EX_A, _, seq)           \
    /***/

#endif


