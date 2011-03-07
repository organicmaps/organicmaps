#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    /// \file poly_function.hpp
    /// A wrapper that makes a tr1-style function object that handles const
    /// and non-const refs and reference_wrapper arguments, too, and forwards
    /// the arguments on to the specified implementation.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #ifndef BOOST_PROTO_DETAIL_POLY_FUNCTION_EAN_2008_05_02
    #define BOOST_PROTO_DETAIL_POLY_FUNCTION_EAN_2008_05_02

    #include <boost/ref.hpp>
    #include <boost/mpl/bool.hpp>
    #include <boost/mpl/void.hpp>
    #include <boost/mpl/size_t.hpp>
    #include <boost/mpl/eval_if.hpp>
    #include <boost/preprocessor/cat.hpp>
    #include <boost/preprocessor/facilities/intercept.hpp>
    #include <boost/preprocessor/iteration/iterate.hpp>
    #include <boost/preprocessor/repetition/enum.hpp>
    #include <boost/preprocessor/repetition/enum_params.hpp>
    #include <boost/preprocessor/repetition/enum_trailing_params.hpp>
    #include <boost/preprocessor/repetition/enum_binary_params.hpp>
    #include <boost/proto/proto_fwd.hpp>

    #ifdef _MSC_VER
    # pragma warning(push)
    # pragma warning(disable: 4181) // const applied to reference type
    #endif

    namespace boost { namespace proto { namespace detail
    {

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename T>
        struct normalize_arg
        {
            typedef T type;
            typedef T const &reference;
        };

        template<typename T>
        struct normalize_arg<T &>
        {
            typedef T type;
            typedef T const &reference;
        };

        template<typename T>
        struct normalize_arg<T const &>
        {
            typedef T type;
            typedef T const &reference;
        };

        template<typename T>
        struct normalize_arg<boost::reference_wrapper<T> >
        {
            typedef T &type;
            typedef T &reference;
        };

        template<typename T>
        struct normalize_arg<boost::reference_wrapper<T> &>
        {
            typedef T &type;
            typedef T &reference;
        };

        template<typename T>
        struct normalize_arg<boost::reference_wrapper<T> const &>
        {
            typedef T &type;
            typedef T &reference;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename T>
        struct arg
        {
            typedef T const &type;

            arg(type t)
              : value(t)
            {}

            operator type() const
            {
                return this->value;
            }

            type operator()() const
            {
                return *this;
            }

        private:
            arg &operator =(arg const &);
            type value;
        };

        template<typename T>
        struct arg<T &>
        {
            typedef T &type;

            arg(type t)
              : value(t)
            {}

            operator type() const
            {
                return this->value;
            }

            type operator()() const
            {
                return *this;
            }

        private:
            arg &operator =(arg const &);
            type value;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename T, typename Void = void>
        struct is_poly_function
          : mpl::false_
        {};

        template<typename T>
        struct is_poly_function<T, typename T::is_poly_function_base_>
          : mpl::true_
        {};

        ////////////////////////////////////////////////////////////////////////////////////////////////
        #define BOOST_PROTO_POLY_FUNCTION()                                                         \
            typedef void is_poly_function_base_;                                                    \
            /**/

        ////////////////////////////////////////////////////////////////////////////////////////////////
        struct poly_function_base
        {
            /// INTERNAL ONLY
            BOOST_PROTO_POLY_FUNCTION()
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename Derived, typename NullaryResult = void>
        struct poly_function
          : poly_function_base
        {
            template<typename Sig>
            struct result;

            template<typename This>
            struct result<This()>
              : Derived::template impl<>
            {
                typedef typename result::result_type type;
            };

            NullaryResult operator()() const
            {
                result<Derived const()> impl;
                return impl();
            }

            #define BOOST_PP_ITERATION_PARAMS_1 (4, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/poly_function.hpp>, 0))
            #include BOOST_PP_ITERATE()
        };

        template<typename T>
        struct wrap_t;

        typedef char poly_function_t;
        typedef char (&mono_function_t)[2];
        typedef char (&unknown_function_t)[3];

        template<typename T> poly_function_t test_poly_function(T *, wrap_t<typename T::is_poly_function_base_> * = 0);
        template<typename T> mono_function_t test_poly_function(T *, wrap_t<typename T::result_type> * = 0);
        template<typename T> unknown_function_t test_poly_function(T *, ...);

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename Fun, typename Sig, typename Switch = mpl::size_t<sizeof(test_poly_function<Fun>(0,0))> >
        struct poly_function_traits
        {
            typedef typename Fun::template result<Sig>::type result_type;
            typedef Fun function_type;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename Fun, typename Sig>
        struct poly_function_traits<Fun, Sig, mpl::size_t<sizeof(mono_function_t)> >
        {
            typedef typename Fun::result_type result_type;
            typedef Fun function_type;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFunSig, bool IsPolyFunction>
        struct as_mono_function_impl;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFunSig>
        struct as_mono_function;

        #define BOOST_PP_ITERATION_PARAMS_1 (4, (1, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/poly_function.hpp>, 1))
        #include BOOST_PP_ITERATE()

    }}} // namespace boost::proto::detail

    #ifdef _MSC_VER
    # pragma warning(pop)
    #endif

    #endif

#elif 0 == BOOST_PP_ITERATION_FLAGS()

    #define N BOOST_PP_ITERATION()

            template<typename This BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
            struct result<This(BOOST_PP_ENUM_PARAMS(N, A))>
              : Derived::template impl<
                    BOOST_PP_ENUM_BINARY_PARAMS(
                        N
                      , typename normalize_arg<A
                      , >::type BOOST_PP_INTERCEPT
                    )
                >
            {
                typedef typename result::result_type type;
            };

            template<BOOST_PP_ENUM_PARAMS(N, typename A)>
            typename result<
                Derived const(
                    BOOST_PP_ENUM_BINARY_PARAMS(N, A, const & BOOST_PP_INTERCEPT)
                )
            >::type
            operator ()(BOOST_PP_ENUM_BINARY_PARAMS(N, A, const &a)) const
            {
                result<
                    Derived const(
                        BOOST_PP_ENUM_BINARY_PARAMS(N, A, const & BOOST_PP_INTERCEPT)
                    )
                > impl;

                #define M0(Z, N, DATA)                                                              \
                    static_cast<typename normalize_arg<BOOST_PP_CAT(A, N) const &>                  \
                        ::reference>(BOOST_PP_CAT(a, N))
                return impl(BOOST_PP_ENUM(N, M0, ~));
                #undef M0
            }

    #undef N

#elif 1 == BOOST_PP_ITERATION_FLAGS()

    #define N BOOST_PP_ITERATION()

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct poly_function_traits<PolyFun, PolyFun(BOOST_PP_ENUM_PARAMS(N, A)), mpl::size_t<sizeof(poly_function_t)> >
        {
            typedef typename PolyFun::template impl<BOOST_PP_ENUM_PARAMS(N, const A)> function_type;
            typedef typename function_type::result_type result_type;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct as_mono_function_impl<PolyFun(BOOST_PP_ENUM_PARAMS(N, A)), true>
        {
            typedef typename PolyFun::template impl<BOOST_PP_ENUM_PARAMS(N, const A)> type;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct as_mono_function_impl<PolyFun(BOOST_PP_ENUM_PARAMS(N, A)), false>
        {
            typedef PolyFun type;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename PolyFun BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
        struct as_mono_function<PolyFun(BOOST_PP_ENUM_PARAMS(N, A))>
          : as_mono_function_impl<PolyFun(BOOST_PP_ENUM_PARAMS(N, A)), is_poly_function<PolyFun>::value>
        {};

    #undef N

#endif
