#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    /// \file fold.hpp
    /// Contains definition of the fold<> and reverse_fold<> transforms.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #ifndef BOOST_PROTO_TRANSFORM_FOLD_HPP_EAN_11_04_2007
    #define BOOST_PROTO_TRANSFORM_FOLD_HPP_EAN_11_04_2007

    #include <boost/version.hpp>
    #include <boost/preprocessor/cat.hpp>
    #include <boost/preprocessor/iteration/iterate.hpp>
    #include <boost/preprocessor/arithmetic/inc.hpp>
    #include <boost/preprocessor/arithmetic/sub.hpp>
    #include <boost/preprocessor/repetition/repeat.hpp>
    #include <boost/fusion/include/fold.hpp>
    #include <boost/proto/proto_fwd.hpp>
    #include <boost/proto/fusion.hpp>
    #include <boost/proto/traits.hpp>
    #include <boost/proto/transform/call.hpp>
    #include <boost/proto/transform/impl.hpp>

    namespace boost { namespace proto
    {
        namespace detail
        {
            template<typename Transform, typename Data>
            struct as_callable
            {
                as_callable(Data d)
                  : d_(d)
                {}

                template<typename Sig>
                struct result;

                #if BOOST_VERSION >= 104200

                template<typename This, typename State, typename Expr>
                struct result<This(State, Expr)>
                {
                    typedef
                        typename when<_, Transform>::template impl<Expr, State, Data>::result_type
                    type;
                };

                template<typename State, typename Expr>
                typename when<_, Transform>::template impl<Expr &, State const &, Data>::result_type
                operator ()(State const &s, Expr &e) const
                {
                    return typename when<_, Transform>::template impl<Expr &, State const &, Data>()(e, s, this->d_);
                }

                #else

                template<typename This, typename Expr, typename State>
                struct result<This(Expr, State)>
                {
                    typedef
                        typename when<_, Transform>::template impl<Expr, State, Data>::result_type
                    type;
                };

                template<typename Expr, typename State>
                typename when<_, Transform>::template impl<Expr &, State const &, Data>::result_type
                operator ()(Expr &e, State const &s) const
                {
                    return typename when<_, Transform>::template impl<Expr &, State const &, Data>()(e, s, this->d_);
                }

                #endif

            private:
                Data d_;
            };

            template<
                typename State0
              , typename Fun
              , typename Expr
              , typename State
              , typename Data
              , long Arity = arity_of<Expr>::value
            >
            struct fold_impl
            {};

            template<
                typename State0
              , typename Fun
              , typename Expr
              , typename State
              , typename Data
              , long Arity = arity_of<Expr>::value
            >
            struct reverse_fold_impl
            {};

            #define BOOST_PROTO_CHILD_N_TYPE(N)\
                BOOST_PP_CAT(proto_child, N)\
                /**/

            #define BOOST_PROTO_FOLD_STATE_TYPE(Z, N, DATA)                                     \
                typedef                                                                         \
                    typename when<_, Fun>::template impl<                                       \
                        typename result_of::child_c<Expr, N>::type                              \
                      , BOOST_PP_CAT(state, N)                                                  \
                      , Data                                                                    \
                    >::result_type                                                              \
                BOOST_PP_CAT(state, BOOST_PP_INC(N));                                           \
                /**/

            #define BOOST_PROTO_FOLD_STATE(Z, N, DATA)                                          \
                BOOST_PP_CAT(state, BOOST_PP_INC(N))                                            \
                BOOST_PP_CAT(s, BOOST_PP_INC(N))                                                \
                  = typename when<_, Fun>::template impl<                                       \
                        typename result_of::child_c<Expr, N>::type                              \
                      , BOOST_PP_CAT(state, N)                                                  \
                      , Data                                                                    \
                    >()(                                                                        \
                        proto::child_c<N>(e)                                                 \
                      , BOOST_PP_CAT(s, N)                                                      \
                      , d                                                                    \
                    );                                                                          \
                /**/

            #define BOOST_PROTO_REVERSE_FOLD_STATE_TYPE(Z, N, DATA)                             \
                typedef                                                                         \
                    typename when<_, Fun>::template impl<                                       \
                        typename result_of::child_c<                                            \
                            Expr                                                                \
                          , BOOST_PP_SUB(DATA, BOOST_PP_INC(N))                                 \
                        >::type                                                                 \
                      , BOOST_PP_CAT(state, BOOST_PP_SUB(DATA, N))                              \
                      , Data                                                                    \
                    >::result_type                                                              \
                BOOST_PP_CAT(state, BOOST_PP_SUB(DATA, BOOST_PP_INC(N)));                       \
                /**/

            #define BOOST_PROTO_REVERSE_FOLD_STATE(Z, N, DATA)                                  \
                BOOST_PP_CAT(state, BOOST_PP_SUB(DATA, BOOST_PP_INC(N)))                        \
                BOOST_PP_CAT(s, BOOST_PP_SUB(DATA, BOOST_PP_INC(N)))                            \
                  = typename when<_, Fun>::template impl<                                       \
                        typename result_of::child_c<                                            \
                            Expr                                                                \
                          , BOOST_PP_SUB(DATA, BOOST_PP_INC(N))                                 \
                        >::type                                                                 \
                      , BOOST_PP_CAT(state, BOOST_PP_SUB(DATA, N))                              \
                      , Data                                                                    \
                    >()(                                                                        \
                        proto::child_c<BOOST_PP_SUB(DATA, BOOST_PP_INC(N))>(e)               \
                      , BOOST_PP_CAT(s, BOOST_PP_SUB(DATA, N))                                  \
                      , d                                                                    \
                    );                                                                          \
                /**/

            #define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/fold.hpp>))
            #include BOOST_PP_ITERATE()

            #undef BOOST_PROTO_REVERSE_FOLD_STATE
            #undef BOOST_PROTO_REVERSE_FOLD_STATE_TYPE
            #undef BOOST_PROTO_FOLD_STATE
            #undef BOOST_PROTO_FOLD_STATE_TYPE
            #undef BOOST_PROTO_CHILD_N_TYPE

        } // namespace detail

        /// \brief A PrimitiveTransform that invokes the <tt>fusion::fold\<\></tt>
        /// algorithm to accumulate
        template<typename Sequence, typename State0, typename Fun>
        struct fold : transform<fold<Sequence, State0, Fun> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                /// \brief A Fusion sequence.
                typedef
                    typename remove_reference<
                        typename when<_, Sequence>::template impl<Expr, State, Data>::result_type
                    >::type
                sequence;

                /// \brief An initial state for the fold.
                typedef
                    typename remove_reference<
                        typename when<_, State0>::template impl<Expr, State, Data>::result_type
                    >::type
                state0;

                /// \brief <tt>fun(d)(e,s) == when\<_,Fun\>()(e,s,d)</tt>
                typedef
                    detail::as_callable<Fun, Data>
                fun;

                typedef
                    typename fusion::result_of::fold<
                        sequence
                      , state0
                      , fun
                    >::type
                result_type;

                /// Let \c seq be <tt>when\<_, Sequence\>()(e, s, d)</tt>, let
                /// \c state0 be <tt>when\<_, State0\>()(e, s, d)</tt>, and
                /// let \c fun(d) be an object such that <tt>fun(d)(e, s)</tt>
                /// is equivalent to <tt>when\<_, Fun\>()(e, s, d)</tt>. Then, this
                /// function returns <tt>fusion::fold(seq, state0, fun(d))</tt>.
                ///
                /// \param e The current expression
                /// \param s The current state
                /// \param d An arbitrary data
                result_type operator ()(
                    typename impl::expr_param   e
                  , typename impl::state_param  s
                  , typename impl::data_param   d
                ) const
                {
                    typename when<_, Sequence>::template impl<Expr, State, Data> seq;
                    detail::as_callable<Fun, Data> f(d);
                    return fusion::fold(
                        seq(e, s, d)
                      , typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d)
                      , f
                    );
                }
            };
        };

        /// \brief A PrimitiveTransform that is the same as the
        /// <tt>fold\<\></tt> transform, except that it folds
        /// back-to-front instead of front-to-back. It uses
        /// the \c _reverse callable PolymorphicFunctionObject
        /// to create a <tt>fusion::reverse_view\<\></tt> of the
        /// sequence before invoking <tt>fusion::fold\<\></tt>.
        template<typename Sequence, typename State0, typename Fun>
        struct reverse_fold
          : fold<call<_reverse(Sequence)>, State0, Fun>
        {};

        // This specialization is only for improved compile-time performance
        // in the commom case when the Sequence transform is \c proto::_.
        //
        /// INTERNAL ONLY
        ///
        template<typename State0, typename Fun>
        struct fold<_, State0, Fun> : transform<fold<_, State0, Fun> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : detail::fold_impl<State0, Fun, Expr, State, Data>
            {};
        };

        // This specialization is only for improved compile-time performance
        // in the commom case when the Sequence transform is \c proto::_.
        //
        /// INTERNAL ONLY
        ///
        template<typename State0, typename Fun>
        struct reverse_fold<_, State0, Fun> : transform<reverse_fold<_, State0, Fun> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : detail::reverse_fold_impl<State0, Fun, Expr, State, Data>
            {};
        };

        /// INTERNAL ONLY
        ///
        template<typename Sequence, typename State, typename Fun>
        struct is_callable<fold<Sequence, State, Fun> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        ///
        template<typename Sequence, typename State, typename Fun>
        struct is_callable<reverse_fold<Sequence, State, Fun> >
          : mpl::true_
        {};

    }}

    #endif

#else

    #define N BOOST_PP_ITERATION()

            template<typename State0, typename Fun, typename Expr, typename State, typename Data>
            struct fold_impl<State0, Fun, Expr, State, Data, N>
              : transform_impl<Expr, State, Data>
            {
                typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
                BOOST_PP_REPEAT(N, BOOST_PROTO_FOLD_STATE_TYPE, N)
                typedef BOOST_PP_CAT(state, N) result_type;

                result_type operator ()(
                    typename fold_impl::expr_param e
                  , typename fold_impl::state_param s
                  , typename fold_impl::data_param d
                ) const
                {
                    state0 s0 =
                        typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
                    BOOST_PP_REPEAT(N, BOOST_PROTO_FOLD_STATE, N)
                    return BOOST_PP_CAT(s, N);
                }
            };

            template<typename State0, typename Fun, typename Expr, typename State, typename Data>
            struct reverse_fold_impl<State0, Fun, Expr, State, Data, N>
              : transform_impl<Expr, State, Data>
            {
                typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type BOOST_PP_CAT(state, N);
                BOOST_PP_REPEAT(N, BOOST_PROTO_REVERSE_FOLD_STATE_TYPE, N)
                typedef state0 result_type;

                result_type operator ()(
                    typename reverse_fold_impl::expr_param e
                  , typename reverse_fold_impl::state_param s
                  , typename reverse_fold_impl::data_param d
                ) const
                {
                    BOOST_PP_CAT(state, N) BOOST_PP_CAT(s, N) =
                        typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
                    BOOST_PP_REPEAT(N, BOOST_PROTO_REVERSE_FOLD_STATE, N)
                    return s0;
                }
            };

    #undef N

#endif
