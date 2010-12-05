///////////////////////////////////////////////////////////////////////////////
/// \file impl.hpp
/// Contains definition of transform<> and transform_impl<> helpers.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_IMPL_HPP_EAN_04_03_2008
#define BOOST_PROTO_TRANSFORM_IMPL_HPP_EAN_04_03_2008

#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace proto
{
    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_TRANSFORM_(PrimitiveTransform, X)                                                       \
    BOOST_PROTO_CALLABLE()                                                                                      \
    typedef X proto_is_transform_;                                                                              \
    typedef PrimitiveTransform transform_type;                                                                  \
                                                                                                                \
    template<typename Sig>                                                                                      \
    struct result                                                                                               \
    {                                                                                                           \
        typedef typename boost::proto::detail::apply_transform<Sig>::result_type type;                          \
    };                                                                                                          \
                                                                                                                \
    template<typename Expr>                                                                                     \
    typename boost::proto::detail::apply_transform<transform_type(Expr &)>::result_type                         \
    operator ()(Expr &e) const                                                                                  \
    {                                                                                                           \
        int i = 0;                                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &)>()(e, i, i);                        \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State &)>::result_type                \
    operator ()(Expr &e, State &s) const                                                                        \
    {                                                                                                           \
        int i = 0;                                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State &)>()(e, s, i);               \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State>                                                                     \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State const &)>::result_type          \
    operator ()(Expr &e, State const &s) const                                                                  \
    {                                                                                                           \
        int i = 0;                                                                                              \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State const &)>()(e, s, i);         \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State &, Data &)>::result_type        \
    operator ()(Expr &e, State &s, Data &d) const                                                               \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State &, Data &)>()(e, s, d);       \
    }                                                                                                           \
                                                                                                                \
    template<typename Expr, typename State, typename Data>                                                      \
    typename boost::proto::detail::apply_transform<transform_type(Expr &, State const &, Data &)>::result_type  \
    operator ()(Expr &e, State const &s, Data &d) const                                                         \
    {                                                                                                           \
        return boost::proto::detail::apply_transform<transform_type(Expr &, State const &, Data &)>()(e, s, d); \
    }                                                                                                           \
    /**/

    #define BOOST_PROTO_TRANSFORM(PrimitiveTransform)                                                           \
        BOOST_PROTO_TRANSFORM_(PrimitiveTransform, void)                                                        \
        /**/

    namespace detail
    {
        template<typename Sig>
        struct apply_transform;

        template<typename PrimitiveTransform, typename Expr>
        struct apply_transform<PrimitiveTransform(Expr)>
          : PrimitiveTransform::template impl<Expr, int, int>
        {};

        template<typename PrimitiveTransform, typename Expr, typename State>
        struct apply_transform<PrimitiveTransform(Expr, State)>
          : PrimitiveTransform::template impl<Expr, State, int>
        {};

        template<typename PrimitiveTransform, typename Expr, typename State, typename Data>
        struct apply_transform<PrimitiveTransform(Expr, State, Data)>
          : PrimitiveTransform::template impl<Expr, State, Data>
        {};
    }

    template<typename PrimitiveTransform, typename X>
    struct transform
    {
        BOOST_PROTO_TRANSFORM_(PrimitiveTransform, X)
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State, Data>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State &, Data>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State, Data &>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State &, Data>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data const data;
        typedef Data const &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State, Data &>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State const state;
        typedef State const &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr, State &, Data &>
    {
        typedef Expr const expr;
        typedef Expr const &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

    template<typename Expr, typename State, typename Data>
    struct transform_impl<Expr &, State &, Data &>
    {
        typedef Expr expr;
        typedef Expr &expr_param;
        typedef State state;
        typedef State &state_param;
        typedef Data data;
        typedef Data &data_param;
    };

}} // namespace boost::proto

#endif
