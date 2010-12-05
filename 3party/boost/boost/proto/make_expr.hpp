#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    /// \file make_expr.hpp
    /// Definition of the \c make_expr() and \c unpack_expr() utilities for
    /// building Proto expression nodes from child nodes or from a Fusion
    /// sequence of child nodes, respectively.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #ifndef BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005
    #define BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005

    #include <boost/config.hpp>
    #include <boost/detail/workaround.hpp>
    #include <boost/preprocessor/cat.hpp>
    #include <boost/preprocessor/arithmetic/inc.hpp>
    #include <boost/preprocessor/arithmetic/dec.hpp>
    #include <boost/preprocessor/arithmetic/sub.hpp>
    #include <boost/preprocessor/punctuation/comma_if.hpp>
    #include <boost/preprocessor/iteration/iterate.hpp>
    #include <boost/preprocessor/facilities/intercept.hpp>
    #include <boost/preprocessor/repetition/enum.hpp>
    #include <boost/preprocessor/repetition/enum_params.hpp>
    #include <boost/preprocessor/repetition/enum_binary_params.hpp>
    #include <boost/preprocessor/repetition/enum_shifted_params.hpp>
    #include <boost/preprocessor/repetition/enum_trailing_params.hpp>
    #include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
    #include <boost/preprocessor/repetition/repeat.hpp>
    #include <boost/ref.hpp>
    #include <boost/mpl/if.hpp>
    #include <boost/mpl/assert.hpp>
    #include <boost/mpl/eval_if.hpp>
    #include <boost/utility/enable_if.hpp>
    #include <boost/type_traits/is_same.hpp>
    #include <boost/type_traits/add_const.hpp>
    #include <boost/type_traits/add_reference.hpp>
    #include <boost/type_traits/remove_cv.hpp>
    #include <boost/proto/proto_fwd.hpp>
    #include <boost/proto/traits.hpp>
    #include <boost/proto/domain.hpp>
    #include <boost/proto/generate.hpp>
    #include <boost/fusion/include/begin.hpp>
    #include <boost/fusion/include/next.hpp>
    #include <boost/fusion/include/value_of.hpp>
    #include <boost/fusion/include/size.hpp>
    #include <boost/proto/detail/poly_function.hpp>
    #include <boost/proto/detail/deprecated.hpp>

    #ifdef _MSC_VER
    # pragma warning(push)
    # pragma warning(disable: 4180) // qualifier applied to function type has no meaning; ignored
    #endif

    namespace boost
    {
        /// INTERNAL ONLY
        ///
        namespace fusion
        {
            /// INTERNAL ONLY
            ///
            template<typename Function>
            class unfused_generic;
        }
    }

    namespace boost { namespace proto
    {
    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_AS_CHILD_TYPE(Z, N, DATA)                                                   \
        typename boost::proto::detail::protoify<                                                    \
            BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, DATA), N)                                        \
          , BOOST_PP_TUPLE_ELEM(3, 2, DATA)                                                         \
        >::result_type                                                                              \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_AS_CHILD(Z, N, DATA)                                                        \
        boost::proto::detail::protoify<                                                             \
            BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, DATA), N)                                        \
          , BOOST_PP_TUPLE_ELEM(3, 2, DATA)                                                         \
        >()(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 1, DATA), N))                                       \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE(Z, N, DATA)                                       \
        typedef typename fusion::result_of::next<                                                   \
            BOOST_PP_CAT(fusion_iterator, N)>::type                                                 \
                BOOST_PP_CAT(fusion_iterator, BOOST_PP_INC(N));                                     \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_ITERATORS_TYPE(N)                                                    \
        typedef                                                                                     \
            typename fusion::result_of::begin<Sequence const>::type                                 \
        fusion_iterator0;                                                                           \
        BOOST_PP_REPEAT(BOOST_PP_DEC(N), BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE, fusion_iterator)    \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
        typename add_const<                                                                         \
            typename fusion::result_of::value_of<                                                   \
                BOOST_PP_CAT(fusion_iterator, N)                                                    \
            >::type                                                                                 \
        >::type                                                                                     \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_NEXT_ITERATOR(Z, N, DATA)                                            \
        BOOST_PP_CAT(fusion_iterator, BOOST_PP_INC(N)) BOOST_PP_CAT(it, BOOST_PP_INC(N)) =          \
            fusion::next(BOOST_PP_CAT(it, N));                                                      \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_ITERATORS(N)                                                         \
        fusion_iterator0 it0 = fusion::begin(sequence);                                             \
        BOOST_PP_REPEAT(BOOST_PP_DEC(N), BOOST_PROTO_FUSION_NEXT_ITERATOR, fusion_iterator)         \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AT(Z, N, DATA)                                                       \
        *BOOST_PP_CAT(it, N)                                                                        \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE(Z, N, DATA)                                         \
        typename detail::protoify<                                                                  \
            BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
          , Domain                                                                                  \
        >::result_type                                                                              \
        /**/

    /// INTERNAL ONLY
    ///
    #define BOOST_PROTO_FUSION_AS_CHILD_AT(Z, N, DATA)                                              \
        detail::protoify<                                                                           \
            BOOST_PROTO_FUSION_AT_TYPE(Z, N, DATA)                                                  \
          , Domain                                                                                  \
        >()(BOOST_PROTO_FUSION_AT(Z, N, DATA))                                                      \
        /**/

        namespace detail
        {
            template<typename T, typename Domain>
            struct protoify
              : Domain::template as_expr<T>
            {};

            template<typename T, typename Domain>
            struct protoify<T &, Domain>
              : Domain::template as_child<T>
            {};

            template<typename T, typename Domain>
            struct protoify<boost::reference_wrapper<T>, Domain>
              : Domain::template as_child<T>
            {};

            template<typename T, typename Domain>
            struct protoify<boost::reference_wrapper<T> const, Domain>
              : Domain::template as_child<T>
            {};

            template<typename Tag, typename Domain, typename Sequence, std::size_t Size>
            struct unpack_expr_
            {};

            template<typename Domain, typename Sequence>
            struct unpack_expr_<tag::terminal, Domain, Sequence, 1u>
            {
                typedef
                    typename add_const<
                        typename fusion::result_of::value_of<
                            typename fusion::result_of::begin<Sequence>::type
                        >::type
                    >::type
                terminal_type;

                typedef
                    typename proto::detail::protoify<
                        terminal_type
                      , Domain
                    >::result_type
                type;

                static type const call(Sequence const &sequence)
                {
                    return proto::detail::protoify<terminal_type, Domain>()(fusion::at_c<0>(sequence));
                }
            };

            template<typename Sequence>
            struct unpack_expr_<tag::terminal, deduce_domain, Sequence, 1u>
              : unpack_expr_<tag::terminal, default_domain, Sequence, 1u>
            {};

            template<
                typename Tag
              , typename Domain
                BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(
                    BOOST_PROTO_MAX_ARITY
                  , typename A
                  , = void BOOST_PP_INTERCEPT
                )
              , typename _ = void
            >
            struct make_expr_
            {};

            template<typename Domain, typename A>
            struct make_expr_<tag::terminal, Domain, A
                BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, void BOOST_PP_INTERCEPT)>
            {
                typedef typename proto::detail::protoify<A, Domain>::result_type result_type;

                result_type operator()(typename add_reference<A>::type a) const
                {
                    return proto::detail::protoify<A, Domain>()(a);
                }
            };

            template<typename A>
            struct make_expr_<tag::terminal, deduce_domain, A
                BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, void BOOST_PP_INTERCEPT)>
              : make_expr_<tag::terminal, default_domain, A>
            {};

        #define BOOST_PP_ITERATION_PARAMS_1                                                         \
            (4, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/make_expr.hpp>, 1))                         \
            /**/

        #include BOOST_PP_ITERATE()
        }

        namespace result_of
        {
            /// \brief Metafunction that computes the return type of the
            /// \c make_expr() function, with a domain deduced from the
            /// domains of the children.
            ///
            /// Use the <tt>result_of::make_expr\<\></tt> metafunction to
            /// compute the return type of the \c make_expr() function.
            ///
            /// In this specialization, the domain is deduced from the
            /// domains of the child types. (If
            /// <tt>is_domain\<A0\>::value</tt> is \c true, then another
            /// specialization is selected.)
            template<
                typename Tag
              , BOOST_PP_ENUM_PARAMS(BOOST_PROTO_MAX_ARITY, typename A)
              , typename Void1  // = void
              , typename Void2  // = void
            >
            struct make_expr
            {
                /// Same as <tt>result_of::make_expr\<Tag, D, A0, ... AN\>::type</tt>
                /// where \c D is the deduced domain, which is calculated as follows:
                ///
                /// For each \c x in <tt>[0,N)</tt> (proceeding in order beginning with
                /// <tt>x=0</tt>), if <tt>domain_of\<Ax\>::type</tt> is not
                /// \c default_domain, then \c D is <tt>domain_of\<Ax\>::type</tt>.
                /// Otherwise, \c D is \c default_domain.
                typedef
                    typename detail::make_expr_<
                        Tag
                      , deduce_domain
                        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                    >::result_type
                type;
            };

            /// \brief Metafunction that computes the return type of the
            /// \c make_expr() function, within the specified domain.
            ///
            /// Use the <tt>result_of::make_expr\<\></tt> metafunction to compute
            /// the return type of the \c make_expr() function.
            template<
                typename Tag
              , typename Domain
                BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, typename A)
            >
            struct make_expr<
                Tag
              , Domain
                BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
              , typename Domain::proto_is_domain_
            >
            {
                /// If \c Tag is <tt>tag::terminal</tt>, then \c type is a
                /// typedef for <tt>boost::result_of\<Domain(expr\<tag::terminal,
                /// term\<A0\> \>)\>::type</tt>.
                ///
                /// Otherwise, \c type is a typedef for <tt>boost::result_of\<Domain(expr\<Tag,
                /// listN\< as_child\<A0\>::type, ... as_child\<AN\>::type\>)
                /// \>::type</tt>, where \c N is the number of non-void template
                /// arguments, and <tt>as_child\<A\>::type</tt> is evaluated as
                /// follows:
                ///
                /// \li If <tt>is_expr\<A\>::value</tt> is \c true, then the
                /// child type is \c A.
                /// \li If \c A is <tt>B &</tt> or <tt>cv boost::reference_wrapper\<B\></tt>,
                /// and <tt>is_expr\<B\>::value</tt> is \c true, then the
                /// child type is <tt>B &</tt>.
                /// \li If <tt>is_expr\<A\>::value</tt> is \c false, then the
                /// child type is <tt>boost::result_of\<Domain(expr\<tag::terminal, term\<A\> \>
                /// )\>::type</tt>.
                /// \li If \c A is <tt>B &</tt> or <tt>cv boost::reference_wrapper\<B\></tt>,
                /// and <tt>is_expr\<B\>::value</tt> is \c false, then the
                /// child type is <tt>boost::result_of\<Domain(expr\<tag::terminal, term\<B &\> \>
                /// )\>::type</tt>.
                typedef
                    typename detail::make_expr_<
                        Tag
                      , Domain
                        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                    >::result_type
                type;
            };

            /// \brief Metafunction that computes the return type of the
            /// \c unpack_expr() function, with a domain deduced from the
            /// domains of the children.
            ///
            /// Use the <tt>result_of::unpack_expr\<\></tt> metafunction to
            /// compute the return type of the \c unpack_expr() function.
            ///
            /// \c Sequence is a Fusion Forward Sequence.
            ///
            /// In this specialization, the domain is deduced from the
            /// domains of the child types. (If
            /// <tt>is_domain\<Sequence>::value</tt> is \c true, then another
            /// specialization is selected.)
            template<
                typename Tag
              , typename Sequence
              , typename Void1  // = void
              , typename Void2  // = void
            >
            struct unpack_expr
            {
                /// Let \c S be the type of a Fusion Random Access Sequence
                /// equivalent to \c Sequence. Then \c type is the
                /// same as <tt>result_of::make_expr\<Tag,
                /// fusion::result_of::value_at_c\<S, 0\>::type, ...
                /// fusion::result_of::value_at_c\<S, N-1\>::type\>::type</tt>,
                /// where \c N is the size of \c S.
                typedef
                    typename detail::unpack_expr_<
                        Tag
                      , deduce_domain
                      , Sequence
                      , fusion::result_of::size<Sequence>::type::value
                    >::type
                type;
            };

            /// \brief Metafunction that computes the return type of the
            /// \c unpack_expr() function, within the specified domain.
            ///
            /// Use the <tt>result_of::make_expr\<\></tt> metafunction to compute
            /// the return type of the \c make_expr() function.
            template<typename Tag, typename Domain, typename Sequence>
            struct unpack_expr<Tag, Domain, Sequence, typename Domain::proto_is_domain_>
            {
                /// Let \c S be the type of a Fusion Random Access Sequence
                /// equivalent to \c Sequence. Then \c type is the
                /// same as <tt>result_of::make_expr\<Tag, Domain,
                /// fusion::result_of::value_at_c\<S, 0\>::type, ...
                /// fusion::result_of::value_at_c\<S, N-1\>::type\>::type</tt>,
                /// where \c N is the size of \c S.
                typedef
                    typename detail::unpack_expr_<
                        Tag
                      , Domain
                      , Sequence
                      , fusion::result_of::size<Sequence>::type::value
                    >::type
                type;
            };
        }

        namespace functional
        {
            /// \brief A callable function object equivalent to the
            /// \c proto::make_expr() function.
            ///
            /// In all cases, <tt>functional::make_expr\<Tag, Domain\>()(a0, ... aN)</tt>
            /// is equivalent to <tt>proto::make_expr\<Tag, Domain\>(a0, ... aN)</tt>.
            ///
            /// <tt>functional::make_expr\<Tag\>()(a0, ... aN)</tt>
            /// is equivalent to <tt>proto::make_expr\<Tag\>(a0, ... aN)</tt>.
            template<typename Tag, typename Domain  /* = deduce_domain*/>
            struct make_expr
            {
                BOOST_PROTO_CALLABLE()
                BOOST_PROTO_POLY_FUNCTION()

                template<typename Sig>
                struct result;

                template<typename This, typename A0>
                struct result<This(A0)>
                {
                    typedef
                        typename result_of::make_expr<
                            Tag
                          , Domain
                          , A0
                        >::type
                    type;
                };

                /// Construct an expression node with tag type \c Tag
                /// and in the domain \c Domain.
                ///
                /// \return <tt>proto::make_expr\<Tag, Domain\>(a0,...aN)</tt>
                template<typename A0>
                typename result_of::make_expr<
                    Tag
                  , Domain
                  , A0 const
                >::type const
                operator ()(A0 const &a0) const
                {
                    return proto::detail::make_expr_<
                        Tag
                      , Domain
                      , A0 const
                    >()(a0);
                }

                // Additional overloads generated by the preprocessor ...

            #define BOOST_PP_ITERATION_PARAMS_1                                                     \
                (4, (2, BOOST_PROTO_MAX_ARITY, <boost/proto/make_expr.hpp>, 2))                     \
                /**/

            #include BOOST_PP_ITERATE()

                /// INTERNAL ONLY
                ///
                template<
                    BOOST_PP_ENUM_BINARY_PARAMS(
                        BOOST_PROTO_MAX_ARITY
                      , typename A
                      , = void BOOST_PP_INTERCEPT
                    )
                >
                struct impl
                  : detail::make_expr_<
                      Tag
                    , Domain
                      BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PROTO_MAX_ARITY, A)
                    >
                {};
            };

            /// \brief A callable function object equivalent to the
            /// \c proto::unpack_expr() function.
            ///
            /// In all cases, <tt>functional::unpack_expr\<Tag, Domain\>()(seq)</tt>
            /// is equivalent to <tt>proto::unpack_expr\<Tag, Domain\>(seq)</tt>.
            ///
            /// <tt>functional::unpack_expr\<Tag\>()(seq)</tt>
            /// is equivalent to <tt>proto::unpack_expr\<Tag\>(seq)</tt>.
            template<typename Tag, typename Domain /* = deduce_domain*/>
            struct unpack_expr
            {
                BOOST_PROTO_CALLABLE()

                template<typename Sig>
                struct result
                {};

                template<typename This, typename Sequence>
                struct result<This(Sequence)>
                {
                    typedef
                        typename result_of::unpack_expr<
                            Tag
                          , Domain
                          , typename remove_reference<Sequence>::type
                        >::type
                    type;
                };

                /// Construct an expression node with tag type \c Tag
                /// and in the domain \c Domain.
                ///
                /// \param sequence A Fusion Forward Sequence
                /// \return <tt>proto::unpack_expr\<Tag, Domain\>(sequence)</tt>
                template<typename Sequence>
                typename result_of::unpack_expr<Tag, Domain, Sequence const>::type const
                operator ()(Sequence const &sequence) const
                {
                    return proto::detail::unpack_expr_<
                        Tag
                      , Domain
                      , Sequence const
                      , fusion::result_of::size<Sequence>::type::value
                    >::call(sequence);
                }
            };

            /// INTERNAL ONLY
            ///
            template<typename Tag, typename Domain>
            struct unfused_expr_fun
            {
                BOOST_PROTO_CALLABLE()

                template<typename Sig>
                struct result;

                template<typename This, typename Sequence>
                struct result<This(Sequence)>
                {
                    typedef
                        typename result_of::unpack_expr<
                            Tag
                          , Domain
                          , typename remove_reference<Sequence>::type
                        >::type
                    type;
                };

                template<typename Sequence>
                typename proto::result_of::unpack_expr<Tag, Domain, Sequence const>::type const
                operator ()(Sequence const &sequence) const
                {
                    return proto::detail::unpack_expr_<
                        Tag
                      , Domain
                      , Sequence const
                      , fusion::result_of::size<Sequence>::type::value
                    >::call(sequence);
                }
            };

            /// INTERNAL ONLY
            ///
            template<typename Tag, typename Domain>
            struct unfused_expr
              : fusion::unfused_generic<unfused_expr_fun<Tag, Domain> >
            {
                BOOST_PROTO_CALLABLE()
            };

        } // namespace functional

        /// \brief Construct an expression of the requested tag type
        /// with a domain and with the specified arguments as children.
        ///
        /// This function template may be invoked either with or without
        /// specifying a \c Domain argument. If no domain is specified,
        /// the domain is deduced by examining in order the domains of
        /// the given arguments and taking the first that is not
        /// \c default_domain, if any such domain exists, or
        /// \c default_domain otherwise.
        ///
        /// Let \c wrap_(x) be defined such that:
        /// \li If \c x is a <tt>boost::reference_wrapper\<\></tt>,
        /// \c wrap_(x) is equivalent to <tt>as_child\<Domain\>(x.get())</tt>.
        /// \li Otherwise, \c wrap_(x) is equivalent to
        /// <tt>as_expr\<Domain\>(x)</tt>.
        ///
        /// Let <tt>make_\<Tag\>(b0,...bN)</tt> be defined as
        /// <tt>expr\<Tag, listN\<C0,...CN\> \>::make(c0,...cN)</tt>
        /// where \c Bx is the type of \c bx.
        ///
        /// \return <tt>Domain()(make_\<Tag\>(wrap_(a0),...wrap_(aN)))</tt>.
        template<typename Tag, typename A0>
        typename lazy_disable_if<
            is_domain<A0>
          , result_of::make_expr<
                Tag
              , A0 const
            >
        >::type const
        make_expr(A0 const &a0)
        {
            return proto::detail::make_expr_<
                Tag
              , deduce_domain
              , A0 const
            >()(a0);
        }

        /// \overload
        ///
        template<typename Tag, typename Domain, typename C0>
        typename result_of::make_expr<
            Tag
          , Domain
          , C0 const
        >::type const
        make_expr(C0 const &c0)
        {
            return proto::detail::make_expr_<
                Tag
              , Domain
              , C0 const
            >()(c0);
        }

        // Additional overloads generated by the preprocessor...

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (4, (2, BOOST_PROTO_MAX_ARITY, <boost/proto/make_expr.hpp>, 3))                             \
        /**/

    #include BOOST_PP_ITERATE()

        /// \brief Construct an expression of the requested tag type
        /// with a domain and with childres from the specified Fusion
        /// Forward Sequence.
        ///
        /// This function template may be invoked either with or without
        /// specifying a \c Domain argument. If no domain is specified,
        /// the domain is deduced by examining in order the domains of the
        /// elements of \c sequence and taking the first that is not
        /// \c default_domain, if any such domain exists, or
        /// \c default_domain otherwise.
        ///
        /// Let \c s be a Fusion Random Access Sequence equivalent to \c sequence.
        /// Let <tt>wrap_\<N\>(s)</tt>, where \c s has type \c S, be defined
        /// such that:
        /// \li If <tt>fusion::result_of::value_at_c\<S,N\>::type</tt> is a reference,
        /// <tt>wrap_\<N\>(s)</tt> is equivalent to
        /// <tt>as_child\<Domain\>(fusion::at_c\<N\>(s))</tt>.
        /// \li Otherwise, <tt>wrap_\<N\>(s)</tt> is equivalent to
        /// <tt>as_expr\<Domain\>(fusion::at_c\<N\>(s))</tt>.
        ///
        /// Let <tt>make_\<Tag\>(b0,...bN)</tt> be defined as
        /// <tt>expr\<Tag, listN\<B0,...BN\> \>::make(b0,...bN)</tt>
        /// where \c Bx is the type of \c bx.
        ///
        /// \param sequence a Fusion Forward Sequence.
        /// \return <tt>Domain()(make_\<Tag\>(wrap_\<0\>(s),...wrap_\<N-1\>(s)))</tt>,
        /// where N is the size of \c Sequence.
        template<typename Tag, typename Sequence>
        typename lazy_disable_if<
            is_domain<Sequence>
          , result_of::unpack_expr<Tag, Sequence const>
        >::type const
        unpack_expr(Sequence const &sequence)
        {
            return proto::detail::unpack_expr_<
                Tag
              , deduce_domain
              , Sequence const
              , fusion::result_of::size<Sequence>::type::value
            >::call(sequence);
        }

        /// \overload
        ///
        template<typename Tag, typename Domain, typename Sequence2>
        typename result_of::unpack_expr<Tag, Domain, Sequence2 const>::type const
        unpack_expr(Sequence2 const &sequence2)
        {
            return proto::detail::unpack_expr_<
                Tag
              , Domain
              , Sequence2 const
              , fusion::result_of::size<Sequence2>::type::value
            >::call(sequence2);
        }

        /// INTERNAL ONLY
        ///
        template<typename Tag, typename Domain>
        struct is_callable<functional::make_expr<Tag, Domain> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        ///
        template<typename Tag, typename Domain>
        struct is_callable<functional::unpack_expr<Tag, Domain> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        ///
        template<typename Tag, typename Domain>
        struct is_callable<functional::unfused_expr<Tag, Domain> >
          : mpl::true_
        {};

    }}

    #ifdef _MSC_VER
    # pragma warning(pop)
    #endif

    #undef BOOST_PROTO_FUSION_AT
    #undef BOOST_PROTO_FUSION_AT_TYPE
    #undef BOOST_PROTO_FUSION_AS_CHILD_AT
    #undef BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE
    #undef BOOST_PROTO_FUSION_NEXT_ITERATOR
    #undef BOOST_PROTO_FUSION_NEXT_ITERATOR_TYPE
    #undef BOOST_PROTO_FUSION_ITERATORS
    #undef BOOST_PROTO_FUSION_ITERATORS_TYPE

    #endif // BOOST_PROTO_MAKE_EXPR_HPP_EAN_04_01_2005

#elif BOOST_PP_ITERATION_FLAGS() == 1

    #define N BOOST_PP_ITERATION()
    #define M BOOST_PP_SUB(BOOST_PROTO_MAX_ARITY, N)

        template<typename Tag, typename Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct make_expr_<Tag, Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
            BOOST_PP_ENUM_TRAILING_PARAMS(M, void BOOST_PP_INTERCEPT), void>
        {
            typedef
                BOOST_PP_CAT(list, N)<
                    BOOST_PP_ENUM(N, BOOST_PROTO_AS_CHILD_TYPE, (A, ~, Domain))
                >
            proto_args;

            typedef typename base_expr<Domain, Tag, proto_args>::type expr_type;
            typedef typename Domain::proto_generator proto_generator;
            typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;

            result_type operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, typename add_reference<A, >::type a)) const
            {
                expr_type const that = {
                    BOOST_PP_ENUM(N, BOOST_PROTO_AS_CHILD, (A, a, Domain))
                };
                return proto_generator()(that);
            }
        };

        template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct make_expr_<Tag, deduce_domain BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
            BOOST_PP_ENUM_TRAILING_PARAMS(M, void BOOST_PP_INTERCEPT), void>
          : make_expr_<
                Tag
              , typename BOOST_PP_CAT(deduce_domain, N)<BOOST_PP_ENUM_PARAMS(N, A)>::type
                BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
            >
        {};

        template<typename Tag, typename Domain, typename Sequence>
        struct unpack_expr_<Tag, Domain, Sequence, N>
        {
            BOOST_PROTO_FUSION_ITERATORS_TYPE(N)

            typedef
                BOOST_PP_CAT(list, N)<
                    BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AS_CHILD_AT_TYPE, ~)
                >
            proto_args;

            typedef typename base_expr<Domain, Tag, proto_args>::type expr_type;
            typedef typename Domain::proto_generator proto_generator;
            typedef typename proto_generator::template result<proto_generator(expr_type)>::type type;

            static type const call(Sequence const &sequence)
            {
                BOOST_PROTO_FUSION_ITERATORS(N)
                expr_type const that = {
                    BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AS_CHILD_AT, ~)
                };
                return proto_generator()(that);
            }
        };

        template<typename Tag, typename Sequence>
        struct unpack_expr_<Tag, deduce_domain, Sequence, N>
        {
            BOOST_PROTO_FUSION_ITERATORS_TYPE(N)

            typedef
                unpack_expr_<
                    Tag
                  , typename BOOST_PP_CAT(deduce_domain, N)<
                        BOOST_PP_ENUM(N, BOOST_PROTO_FUSION_AT_TYPE, ~)
                    >::type
                  , Sequence
                  , N
                >
            other;

            typedef typename other::type type;

            static type const call(Sequence const &sequence)
            {
                return other::call(sequence);
            }
        };

    #undef N
    #undef M

#elif BOOST_PP_ITERATION_FLAGS() == 2

    #define N BOOST_PP_ITERATION()

        template<typename This BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct result<This(BOOST_PP_ENUM_PARAMS(N, A))>
        {
            typedef
                typename result_of::make_expr<
                    Tag
                  , Domain
                    BOOST_PP_ENUM_TRAILING_PARAMS(N, A)
                >::type
            type;
        };

        /// \overload
        ///
        template<BOOST_PP_ENUM_PARAMS(N, typename A)>
        typename result_of::make_expr<
            Tag
          , Domain
            BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
        >::type const
        operator ()(BOOST_PP_ENUM_BINARY_PARAMS(N, const A, &a)) const
        {
            return proto::detail::make_expr_<
                Tag
              , Domain
                BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
            >()(BOOST_PP_ENUM_PARAMS(N, a));
        }

    #undef N

#elif BOOST_PP_ITERATION_FLAGS() == 3

    #define N BOOST_PP_ITERATION()

        /// \overload
        ///
        template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        typename lazy_disable_if<
            is_domain<A0>
          , result_of::make_expr<
                Tag
                BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
            >
        >::type const
        make_expr(BOOST_PP_ENUM_BINARY_PARAMS(N, const A, &a))
        {
            return proto::detail::make_expr_<
                Tag
              , deduce_domain
                BOOST_PP_ENUM_TRAILING_PARAMS(N, const A)
            >()(BOOST_PP_ENUM_PARAMS(N, a));
        }

        /// \overload
        ///
        template<typename Tag, typename Domain BOOST_PP_ENUM_TRAILING_PARAMS(N, typename C)>
        typename result_of::make_expr<
            Tag
          , Domain
            BOOST_PP_ENUM_TRAILING_PARAMS(N, const C)
        >::type const
        make_expr(BOOST_PP_ENUM_BINARY_PARAMS(N, const C, &c))
        {
            return proto::detail::make_expr_<
                Tag
              , Domain
                BOOST_PP_ENUM_TRAILING_PARAMS(N, const C)
            >()(BOOST_PP_ENUM_PARAMS(N, c));
        }

    #undef N

#endif // BOOST_PP_IS_ITERATING
