#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    /// \file make.hpp
    /// Contains definition of the make<> transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #ifndef BOOST_PROTO_TRANSFORM_MAKE_HPP_EAN_12_02_2007
    #define BOOST_PROTO_TRANSFORM_MAKE_HPP_EAN_12_02_2007

    #include <boost/detail/workaround.hpp>
    #include <boost/preprocessor/repetition/enum.hpp>
    #include <boost/preprocessor/repetition/enum_params.hpp>
    #include <boost/preprocessor/repetition/enum_trailing_params.hpp>
    #include <boost/preprocessor/repetition/enum_binary_params.hpp>
    #include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
    #include <boost/preprocessor/repetition/repeat_from_to.hpp>
    #include <boost/preprocessor/facilities/intercept.hpp>
    #include <boost/preprocessor/cat.hpp>
    #include <boost/preprocessor/iteration/iterate.hpp>
    #include <boost/preprocessor/selection/max.hpp>
    #include <boost/preprocessor/arithmetic/inc.hpp>
    #include <boost/mpl/and.hpp>
    #include <boost/mpl/aux_/has_type.hpp>
    #include <boost/mpl/aux_/template_arity.hpp>
    #include <boost/mpl/aux_/lambda_arity_param.hpp>
    #include <boost/utility/result_of.hpp>
    #include <boost/proto/proto_fwd.hpp>
    #include <boost/proto/traits.hpp>
    #include <boost/proto/args.hpp>
    #include <boost/proto/transform/impl.hpp>
    #include <boost/proto/detail/as_lvalue.hpp>
    #include <boost/proto/detail/ignore_unused.hpp>

    namespace boost { namespace proto
    {
        namespace detail
        {
            template<typename T>
            struct is_applyable
              : mpl::and_<is_callable<T>, is_transform<T> >
            {};

            template<typename T, bool HasType = mpl::aux::has_type<T>::value>
            struct nested_type
            {
                typedef typename T::type type;
            };

            template<typename T>
            struct nested_type<T, false>
            {
                typedef T type;
            };

            template<typename T, bool Applied>
            struct nested_type_if
            {
                typedef T type;
                static bool const applied = false;
            };

            template<typename T>
            struct nested_type_if<T, true>
              : nested_type<T>
            {
                static bool const applied = true;
            };

            template<
                typename R
              , typename Expr, typename State, typename Data
                BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(long Arity = mpl::aux::template_arity<R>::value)
            >
            struct make_
            {
                typedef R type;
                static bool const applied = false;
            };

            template<
                typename R
              , typename Expr, typename State, typename Data
              , bool IsApplyable = is_applyable<R>::value
            >
            struct make_if_
              : make_<R, Expr, State, Data>
            {};

            template<typename R, typename Expr, typename State, typename Data>
            struct make_if_<R, Expr, State, Data, true>
              : uncvref<typename when<_, R>::template impl<Expr, State, Data>::result_type>
            {
                static bool const applied = true;
            };

            #if BOOST_WORKAROUND(__GNUC__, == 3) || (__GNUC__ == 4 && __GNUC_MINOR__ == 0)
            // work around GCC bug
            template<typename Tag, typename Args, long N, typename Expr, typename State, typename Data>
            struct make_if_<proto::expr<Tag, Args, N>, Expr, State, Data, false>
            {
                typedef proto::expr<Tag, Args, N> type;
                static bool const applied = false;
            };

            // work around GCC bug
            template<typename Tag, typename Args, long N, typename Expr, typename State, typename Data>
            struct make_if_<proto::basic_expr<Tag, Args, N>, Expr, State, Data, false>
            {
                typedef proto::basic_expr<Tag, Args, N> type;
                static bool const applied = false;
            };
            #endif

            template<typename Type, bool IsAggregate = is_aggregate<Type>::value>
            struct construct_
            {
                typedef Type result_type;

                Type operator ()() const
                {
                    return Type();
                }

                #define TMP(Z, N, DATA)                                                             \
                template<BOOST_PP_ENUM_PARAMS_Z(Z, N, typename A)>                                  \
                Type operator ()(BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, A, &a)) const                  \
                {                                                                                   \
                    return Type(BOOST_PP_ENUM_PARAMS_Z(Z, N, a));                                   \
                }
                BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), TMP, ~)
                #undef TMP
            };

            template<typename Type>
            struct construct_<Type, true>
            {
                typedef Type result_type;

                Type operator ()() const
                {
                    return Type();
                }

                #define TMP(Z, N, DATA)                                                             \
                template<BOOST_PP_ENUM_PARAMS_Z(Z, N, typename A)>                                  \
                Type operator ()(BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, A, &a)) const                  \
                {                                                                                   \
                    Type that = {BOOST_PP_ENUM_PARAMS_Z(Z, N, a)};                                  \
                    return that;                                                                    \
                }
                BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), TMP, ~)
                #undef TMP
            };

            #define TMP(Z, N, DATA)                                                                 \
            template<typename Type BOOST_PP_ENUM_TRAILING_PARAMS_Z(Z, N, typename A)>               \
            Type construct(BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, A, &a))                              \
            {                                                                                       \
                return construct_<Type>()(BOOST_PP_ENUM_PARAMS_Z(Z, N, a));                         \
            }
            BOOST_PP_REPEAT(BOOST_PROTO_MAX_ARITY, TMP, ~)
            #undef TMP
        }

        /// \brief A PrimitiveTransform which prevents another PrimitiveTransform
        /// from being applied in an \c ObjectTransform.
        ///
        /// When building higher order transforms with <tt>make\<\></tt> or
        /// <tt>lazy\<\></tt>, you sometimes would like to build types that
        /// are parameterized with Proto transforms. In such lambda-style
        /// transforms, Proto will unhelpfully find all nested transforms
        /// and apply them, even if you don't want them to be applied. Consider
        /// the following transform, which will replace the \c _ in
        /// <tt>Bar<_>()</tt> with <tt>proto::terminal\<int\>::type</tt>:
        ///
        /// \code
        /// template<typename T>
        /// struct Bar
        /// {};
        /// 
        /// struct Foo
        ///   : proto::when<_, Bar<_>() >
        /// {};
        /// 
        /// proto::terminal<int>::type i = {0};
        /// 
        /// int main()
        /// {
        ///     Foo()(i);
        ///     std::cout << typeid(Foo()(i)).name() << std::endl;
        /// }
        /// \endcode
        ///
        /// If you actually wanted to default-construct an object of type
        /// <tt>Bar\<_\></tt>, you would have to protect the \c _ to prevent
        /// it from being applied. You can use <tt>proto::protect\<\></tt>
        /// as follows:
        ///
        /// \code
        /// // OK: replace anything with Bar<_>()
        /// struct Foo
        ///   : proto::when<_, Bar<protect<_> >() >
        /// {};
        /// \endcode
        template<typename PrimitiveTransform>
        struct protect : transform<protect<PrimitiveTransform> >
        {
            template<typename, typename, typename>
            struct impl
            {
                typedef PrimitiveTransform result_type;
            };
        };

        /// \brief A PrimitiveTransform which computes a type by evaluating any
        /// nested transforms and then constructs an object of that type.
        ///
        /// The <tt>make\<\></tt> transform checks to see if \c Object is a template.
        /// If it is, the template type is disassembled to find nested transforms.
        /// Proto considers the following types to represent transforms:
        ///
        /// \li Function types
        /// \li Function pointer types
        /// \li Types for which <tt>proto::is_callable\< type \>::value</tt> is \c true
        ///
        /// <tt>boost::result_of\<make\<T\<X0,X1,...\> \>(Expr, State, Data)\>::type</tt>
        /// is evaluated as follows. For each \c X in <tt>X0,X1,...</tt>, do:
        ///
        /// \li If \c X is a template like <tt>U\<Y0,Y1,...\></tt>, then let <tt>X'</tt>
        ///     be <tt>boost::result_of\<make\<U\<Y0,Y1,...\> \>(Expr, State, Data)\>::type</tt>
        ///     (which evaluates this procedure recursively). Note whether any
        ///     substitutions took place during this operation.
        /// \li Otherwise, if \c X is a transform, then let <tt>X'</tt> be
        ///     <tt>boost::result_of\<when\<_, X\>(Expr, State, Data)\>::type</tt>.
        ///     Note that a substitution took place.
        /// \li Otherwise, let <tt>X'</tt> be \c X, and note that no substitution
        ///     took place.
        /// \li If any substitutions took place in any of the above steps and
        ///     <tt>T\<X0',X1',...\></tt> has a nested <tt>::type</tt> typedef,
        ///     the result type is <tt>T\<X0',X1',...\>::type</tt>.
        /// \li Otherwise, the result type is <tt>T\<X0',X1',...\></tt>.
        ///
        /// Note that <tt>when\<\></tt> is implemented in terms of <tt>call\<\></tt>
        /// and <tt>make\<\></tt>, so the above procedure is evaluated recursively.
        template<typename Object>
        struct make : transform<make<Object> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;

                /// \return <tt>result_type()</tt>
                result_type operator ()(
                    typename impl::expr_param
                  , typename impl::state_param
                  , typename impl::data_param
                ) const
                {
                    return result_type();
                }
            };
        };

        #define BOOST_PP_ITERATION_PARAMS_1 (3, (0, BOOST_PROTO_MAX_ARITY, <boost/proto/transform/make.hpp>))
        #include BOOST_PP_ITERATE()

        /// INTERNAL ONLY
        ///
        template<typename Object>
        struct is_callable<make<Object> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        ///
        template<typename PrimitiveTransform>
        struct is_callable<protect<PrimitiveTransform> >
          : mpl::true_
        {};

    }}

    #endif

#else

    #define N BOOST_PP_ITERATION()

        namespace detail
        {
            #if N > 0
            #define TMP0(Z, M, DATA) make_if_<BOOST_PP_CAT(A, M), Expr, State, Data>
            #define TMP1(Z, M, DATA) typename TMP0(Z, M, DATA) ::type
            #define TMP2(Z, M, DATA) TMP0(Z, M, DATA) ::applied ||

            template<
                template<BOOST_PP_ENUM_PARAMS(N, typename BOOST_PP_INTERCEPT)> class R
                BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)
              , typename Expr, typename State, typename Data
            >
            struct make_<
                R<BOOST_PP_ENUM_PARAMS(N, A)>
              , Expr, State, Data
                BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(N)
            >
              : nested_type_if<R<BOOST_PP_ENUM(N, TMP1, ~)>, (BOOST_PP_REPEAT(N, TMP2, ~) false)>
            {};

            template<
                template<BOOST_PP_ENUM_PARAMS(N, typename BOOST_PP_INTERCEPT)> class R
                BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)
              , typename Expr, typename State, typename Data
            >
            struct make_<
                noinvoke<R<BOOST_PP_ENUM_PARAMS(N, A)> >
              , Expr, State, Data
                BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(1)
            >
            {
                typedef R<BOOST_PP_ENUM(N, TMP1, ~)> type;
                static bool const applied = true;
            };

            #undef TMP0
            #undef TMP1
            #undef TMP2
            #endif

            template<typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
            struct is_applyable<R(BOOST_PP_ENUM_PARAMS(N, A))>
              : mpl::true_
            {};

            template<typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
            struct is_applyable<R(*)(BOOST_PP_ENUM_PARAMS(N, A))>
              : mpl::true_
            {};

            template<typename T, typename A>
            struct construct_<proto::expr<T, A, N>, true>
            {
                typedef proto::expr<T, A, N> result_type;

                template<BOOST_PP_ENUM_PARAMS(BOOST_PP_MAX(N, 1), typename A)>
                result_type operator ()(BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_MAX(N, 1), A, &a)) const
                {
                    return result_type::make(BOOST_PP_ENUM_PARAMS(BOOST_PP_MAX(N, 1), a));
                }
            };

            template<typename T, typename A>
            struct construct_<proto::basic_expr<T, A, N>, true>
            {
                typedef proto::basic_expr<T, A, N> result_type;

                template<BOOST_PP_ENUM_PARAMS(BOOST_PP_MAX(N, 1), typename A)>
                result_type operator ()(BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_MAX(N, 1), A, &a)) const
                {
                    return result_type::make(BOOST_PP_ENUM_PARAMS(BOOST_PP_MAX(N, 1), a));
                }
            };
        }

        /// \brief A PrimitiveTransform which computes a type by evaluating any
        /// nested transforms and then constructs an object of that type with the
        /// current expression, state and data, transformed according
        /// to \c A0 through \c AN.
        template<typename Object BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct make<Object(BOOST_PP_ENUM_PARAMS(N, A))>
          : transform<make<Object(BOOST_PP_ENUM_PARAMS(N, A))> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                /// \brief <tt>boost::result_of\<make\<Object\>(Expr, State, Data)\>::type</tt>
                typedef typename detail::make_if_<Object, Expr, State, Data>::type result_type;

                /// Let \c ax be <tt>when\<_, Ax\>()(e, s, d)</tt>
                /// for each \c x in <tt>[0,N]</tt>.
                /// Return <tt>result_type(a0, a1,... aN)</tt>.
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
                    proto::detail::ignore_unused(e);
                    proto::detail::ignore_unused(s);
                    proto::detail::ignore_unused(d);
                    return detail::construct<result_type>(
                        #define TMP(Z, M, DATA)                                                     \
                            detail::as_lvalue(                                                      \
                                typename when<_, BOOST_PP_CAT(A, M)>                                \
                                    ::template impl<Expr, State, Data>()(e, s, d)                   \
                            )
                        BOOST_PP_ENUM(N, TMP, DATA)
                        #undef TMP
                    );
                }
            };
        };

        #if BOOST_WORKAROUND(__GNUC__, == 3) || (__GNUC__ == 4 && __GNUC_MINOR__ == 0)
        // work around GCC bug
        template<typename Tag, typename Args, long Arity BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct make<proto::expr<Tag, Args, Arity>(BOOST_PP_ENUM_PARAMS(N, A))>
          : transform<make<proto::expr<Tag, Args, Arity>(BOOST_PP_ENUM_PARAMS(N, A))> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                typedef proto::expr<Tag, Args, Arity> result_type;

                result_type operator ()(
                    typename impl::expr_param   e
                  , typename impl::state_param  s
                  , typename impl::data_param   d
                ) const
                {
                    return proto::expr<Tag, Args, Arity>::make(
                        #define TMP(Z, M, DATA)                                                     \
                            detail::as_lvalue(                                                      \
                                typename when<_, BOOST_PP_CAT(A, M)>                                \
                                    ::template impl<Expr, State, Data>()(e, s, d)                   \
                            )
                        BOOST_PP_ENUM(N, TMP, DATA)
                        #undef TMP
                    );
                }
            };
        };

        template<typename Tag, typename Args, long Arity BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct make<proto::basic_expr<Tag, Args, Arity>(BOOST_PP_ENUM_PARAMS(N, A))>
          : transform<make<proto::basic_expr<Tag, Args, Arity>(BOOST_PP_ENUM_PARAMS(N, A))> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                typedef proto::basic_expr<Tag, Args, Arity> result_type;

                result_type operator ()(
                    typename impl::expr_param   e
                  , typename impl::state_param  s
                  , typename impl::data_param   d
                ) const
                {
                    return proto::basic_expr<Tag, Args, Arity>::make(
                        #define TMP(Z, M, DATA)                                                     \
                            detail::as_lvalue(                                                      \
                                typename when<_, BOOST_PP_CAT(A, M)>                                \
                                    ::template impl<Expr, State, Data>()(e, s, d)                   \
                            )
                        BOOST_PP_ENUM(N, TMP, DATA)
                        #undef TMP
                    );
                }
            };
        };
        #endif

    #undef N

#endif
