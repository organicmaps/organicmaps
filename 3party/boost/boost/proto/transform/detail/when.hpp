#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/transform/detail/preprocessed/when.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/when.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// \file when.hpp
    /// Definition of when transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (0, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/detail/when.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

#else

    #define N BOOST_PP_ITERATION()

    /// \brief A grammar element and a PrimitiveTransform that associates
    /// a transform with the grammar.
    ///
    /// Use <tt>when\<\></tt> to override a grammar's default transform
    /// with a custom transform. It is for used when composing larger
    /// transforms by associating smaller transforms with individual
    /// rules in your grammar, as in the following transform which
    /// counts the number of terminals in an expression.
    ///
    /// \code
    /// // Count the terminals in an expression tree.
    /// // Must be invoked with initial state == mpl::int_<0>().
    /// struct CountLeaves
    ///   : or_<
    ///         when<terminal<_>, mpl::next<_state>()>
    ///       , otherwise<fold<_, _state, CountLeaves> >
    ///     >
    /// {};
    /// \endcode
    ///
    /// The <tt>when\<G, R(A0,A1,...)\></tt> form accepts either a
    /// CallableTransform or an ObjectTransform as its second parameter.
    /// <tt>when\<\></tt> uses <tt>is_callable\<R\>::value</tt> to
    /// distinguish between the two, and uses <tt>call\<\></tt> to
    /// evaluate CallableTransforms and <tt>make\<\></tt> to evaluate
    /// ObjectTransforms.
    template<typename Grammar, typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct when<Grammar, R(BOOST_PP_ENUM_PARAMS(N, A))>
      : transform<when<Grammar, R(BOOST_PP_ENUM_PARAMS(N, A))> >
    {
        typedef Grammar first;
        typedef R second(BOOST_PP_ENUM_PARAMS(N, A));
        typedef typename Grammar::proto_grammar proto_grammar;

        // Note: do not evaluate is_callable<R> in this scope.
        // R may be an incomplete type at this point.

        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            // OK to evaluate is_callable<R> here. R should be compete by now.
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(BOOST_PP_ENUM_PARAMS(N, A))> // "R" is a function to call
                  , make<R(BOOST_PP_ENUM_PARAMS(N, A))> // "R" is an object to construct
                >::type
            which;

            typedef typename which::template impl<Expr, State, Data>::result_type result_type;

            /// Evaluate <tt>R(A0,A1,...)</tt> as a transform either with
            /// <tt>call\<\></tt> or with <tt>make\<\></tt> depending on
            /// whether <tt>is_callable\<R\>::value</tt> is \c true or
            /// \c false.
            ///
            /// \param e The current expression
            /// \param s The current state
            /// \param d An arbitrary data
            /// \pre <tt>matches\<Expr, Grammar\>::value</tt> is \c true
            /// \return <tt>which()(e, s, d)</tt>
            result_type operator ()(
                typename impl::expr_param   e
              , typename impl::state_param  s
              , typename impl::data_param   d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };

    #undef N

#endif
