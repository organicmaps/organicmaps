#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    /// \file pass_through.hpp
    ///
    /// Definition of the pass_through transform, which is the default transform
    /// of all of the expression generator metafunctions such as unary_plus<>, plus<>
    /// and nary_expr<>.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #ifndef BOOST_PROTO_TRANSFORM_PASS_THROUGH_HPP_EAN_12_26_2006
    #define BOOST_PROTO_TRANSFORM_PASS_THROUGH_HPP_EAN_12_26_2006

    #include <boost/preprocessor/cat.hpp>
    #include <boost/preprocessor/repetition/enum.hpp>
    #include <boost/preprocessor/iteration/iterate.hpp>
    #include <boost/mpl/bool.hpp>
    #include <boost/type_traits/remove_reference.hpp>
    #include <boost/proto/proto_fwd.hpp>
    #include <boost/proto/args.hpp>
    #include <boost/proto/transform/impl.hpp>

    namespace boost { namespace proto
    {
        namespace detail
        {
            template<
                typename Grammar
              , typename Expr
              , typename State
              , typename Data
              , long Arity = arity_of<Expr>::value
            >
            struct pass_through_impl
            {};

            #define BOOST_PROTO_DEFINE_TRANSFORM_TYPE(Z, N, DATA)                                   \
                typename Grammar::BOOST_PP_CAT(proto_child, N)::template impl<                      \
                    typename result_of::child_c<Expr, N>::type                                      \
                  , State                                                                           \
                  , Data                                                                            \
                >::result_type                                                                      \
                /**/

            #define BOOST_PROTO_DEFINE_TRANSFORM(Z, N, DATA)                                        \
                typename Grammar::BOOST_PP_CAT(proto_child, N)::template impl<                      \
                    typename result_of::child_c<Expr, N>::type                                      \
                  , State                                                                           \
                  , Data                                                                            \
                >()(                                                                                \
                    e.proto_base().BOOST_PP_CAT(child, N), s, d                                     \
                )                                                                                   \
                /**/

            #define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/pass_through.hpp>))
            #include BOOST_PP_ITERATE()

            #undef BOOST_PROTO_DEFINE_TRANSFORM
            #undef BOOST_PROTO_DEFINE_TRANSFORM_TYPE

            template<typename Grammar, typename Expr, typename State, typename Data>
            struct pass_through_impl<Grammar, Expr, State, Data, 0>
              : transform_impl<Expr, State, Data>
            {
                typedef Expr result_type;

                /// \param e An expression
                /// \return \c e
                /// \throw nothrow
                #ifdef BOOST_PROTO_STRICT_RESULT_OF
                result_type
                #else
                typename pass_through_impl::expr_param
                #endif
                operator()(
                    typename pass_through_impl::expr_param e
                  , typename pass_through_impl::state_param
                  , typename pass_through_impl::data_param
                ) const
                {
                    return e;
                }
            };

        } // namespace detail

        /// \brief A PrimitiveTransform that transforms the child expressions
        /// of an expression node according to the corresponding children of
        /// a Grammar.
        ///
        /// Given a Grammar such as <tt>plus\<T0, T1\></tt>, an expression type
        /// that matches the grammar such as <tt>plus\<E0, E1\>::type</tt>, a
        /// state \c S and a data \c V, the result of applying the
        /// <tt>pass_through\<plus\<T0, T1\> \></tt> transform is:
        ///
        /// \code
        /// plus<
        ///     T0::result<T0(E0, S, V)>::type
        ///   , T1::result<T1(E1, S, V)>::type
        /// >::type
        /// \endcode
        ///
        /// The above demonstrates how child transforms and child expressions
        /// are applied pairwise, and how the results are reassembled into a new
        /// expression node with the same tag type as the original.
        ///
        /// The explicit use of <tt>pass_through\<\></tt> is not usually needed,
        /// since the expression generator metafunctions such as
        /// <tt>plus\<\></tt> have <tt>pass_through\<\></tt> as their default
        /// transform. So, for instance, these are equivalent:
        ///
        /// \code
        /// // Within a grammar definition, these are equivalent:
        /// when< plus<X, Y>, pass_through< plus<X, Y> > >
        /// when< plus<X, Y>, plus<X, Y> >
        /// when< plus<X, Y> > // because of when<class X, class Y=X>
        /// plus<X, Y>         // because plus<> is both a
        ///                    //   grammar and a transform
        /// \endcode
        ///
        /// For example, consider the following transform that promotes all
        /// \c float terminals in an expression to \c double.
        ///
        /// \code
        /// // This transform finds all float terminals in an expression and promotes
        /// // them to doubles.
        /// struct Promote
        ///  : or_<
        ///         when<terminal<float>, terminal<double>::type(_value) >
        ///         // terminal<>'s default transform is a no-op:
        ///       , terminal<_>
        ///         // nary_expr<> has a pass_through<> transform:
        ///       , nary_expr<_, vararg<Promote> >
        ///     >
        /// {};
        /// \endcode
        template<typename Grammar>
        struct pass_through
          : transform<pass_through<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : detail::pass_through_impl<Grammar, Expr, State, Data>
            {};
        };

        /// INTERNAL ONLY
        ///
        template<typename Grammar>
        struct is_callable<pass_through<Grammar> >
          : mpl::true_
        {};

    }} // namespace boost::proto

    #endif

#else

    #define N BOOST_PP_ITERATION()

            template<typename Grammar, typename Expr, typename State, typename Data>
            struct pass_through_impl<Grammar, Expr, State, Data, N>
              : transform_impl<Expr, State, Data>
            {
                typedef typename pass_through_impl::expr unref_expr;

                typedef
                    typename base_expr<
                        typename unref_expr::proto_domain
                      , typename unref_expr::proto_tag
                      , BOOST_PP_CAT(list, N)<
                            BOOST_PP_ENUM(N, BOOST_PROTO_DEFINE_TRANSFORM_TYPE, ~)
                        >
                    >::type
                expr_type;

                typedef typename unref_expr::proto_generator proto_generator;
                typedef typename boost::tr1_result_of<proto_generator(expr_type)>::type result_type;

                result_type const operator ()(
                    typename pass_through_impl::expr_param e
                  , typename pass_through_impl::state_param s
                  , typename pass_through_impl::data_param d
                ) const
                {
                    expr_type const that = {
                        BOOST_PP_ENUM(N, BOOST_PROTO_DEFINE_TRANSFORM, ~)
                    };
                    #if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1400))
                    // Without this, MSVC complains that "that" is uninitialized,
                    // and it actually triggers a runtime check in debug mode when
                    // built with VC8.
                    &that;
                    #endif
                    return proto_generator()(that);
                }
            };

    #undef N

#endif
