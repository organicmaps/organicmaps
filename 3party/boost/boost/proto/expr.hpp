///////////////////////////////////////////////////////////////////////////////
/// \file expr.hpp
/// Contains definition of expr\<\> class template.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_EXPR_HPP_EAN_04_01_2005
#define BOOST_PROTO_EXPR_HPP_EAN_04_01_2005

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/selection/max.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/args.hpp>
#include <boost/proto/traits.hpp>

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma warning(push)
# pragma warning(disable : 4510) // default constructor could not be generated
# pragma warning(disable : 4512) // assignment operator could not be generated
# pragma warning(disable : 4610) // user defined constructor required
#endif

namespace boost { namespace proto
{

    namespace detail
    {
    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_CHILD(Z, N, DATA)                                                       \
        typedef BOOST_PP_CAT(Arg, N) BOOST_PP_CAT(proto_child, N);                              \
        BOOST_PP_CAT(proto_child, N) BOOST_PP_CAT(child, N);                                    \
        /**< INTERNAL ONLY */

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_VOID(Z, N, DATA)                                                        \
        typedef void BOOST_PP_CAT(proto_child, N);                                              \
        /**< INTERNAL ONLY */

        struct not_a_valid_type
        {
        private:
            not_a_valid_type()
            {}
        };
        
        template<typename Tag, typename Arg>
        struct address_of_hack
        {
            typedef not_a_valid_type type;
        };

        template<typename Expr>
        struct address_of_hack<proto::tag::address_of, Expr &>
        {
            typedef Expr *type;
        };

        template<typename T, typename Expr, typename Arg0>
        Expr make_terminal(T &t, Expr *, proto::term<Arg0> *)
        {
            Expr that = {t};
            return that;
        }

        template<typename T, typename Expr, typename Arg0, std::size_t N>
        Expr make_terminal(T (&t)[N], Expr *, proto::term<Arg0[N]> *)
        {
            Expr that;
            for(std::size_t i = 0; i < N; ++i)
            {
                that.child0[i] = t[i];
            }
            return that;
        }

        template<typename T, typename Expr, typename Arg0, std::size_t N>
        Expr make_terminal(T const(&t)[N], Expr *, proto::term<Arg0[N]> *)
        {
            Expr that;
            for(std::size_t i = 0; i < N; ++i)
            {
                that.child0[i] = t[i];
            }
            return that;
        }

        template<typename T, typename U>
        struct same_cv
        {
            typedef U type;
        };

        template<typename T, typename U>
        struct same_cv<T const, U>
        {
            typedef U const type;
        };
    }

    namespace result_of
    {
        /// \brief A helper metafunction for computing the
        /// return type of \c proto::expr\<\>::operator().
        template<typename Sig, typename This, typename Domain>
        struct funop;

        #define BOOST_PP_ITERATION_PARAMS_1 (3, (0, BOOST_PP_DEC(BOOST_PROTO_MAX_FUNCTION_CALL_ARITY), <boost/proto/detail/funop.hpp>))
        #include BOOST_PP_ITERATE()
    }

    namespace exprns_
    {
        // The expr<> specializations are actually defined here.
        #define BOOST_PROTO_DEFINE_TERMINAL
        #define BOOST_PP_ITERATION_PARAMS_1 (3, (0, 0, <boost/proto/detail/expr0.hpp>))
        #include BOOST_PP_ITERATE()

        #undef BOOST_PROTO_DEFINE_TERMINAL
        #define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/expr0.hpp>))
        #include BOOST_PP_ITERATE()
    }

    #undef BOOST_PROTO_CHILD
    #undef BOOST_PROTO_VOID

    /// \brief Lets you inherit the interface of an expression
    /// while hiding from Proto the fact that the type is a Proto
    /// expression.
    template<typename Expr>
    struct unexpr
      : Expr
    {
        BOOST_PROTO_UNEXPR()

        explicit unexpr(Expr const &e)
          : Expr(e)
        {}
        
        using Expr::operator =;
    };

}}

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma warning(pop)
#endif

#endif // BOOST_PROTO_EXPR_HPP_EAN_04_01_2005
