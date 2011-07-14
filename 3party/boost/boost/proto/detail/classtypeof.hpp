#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/classtypeof.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/classtypeof.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    // classtypeof.hpp
    // Contains specializations of the classtypeof\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (0, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/classtypeof.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

#else

    #define N BOOST_PP_ITERATION()

    template<typename T, typename U BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct classtypeof<T (U::*)(BOOST_PP_ENUM_PARAMS(N, A))>
    {
        typedef U type;
    };

    template<typename T, typename U BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct classtypeof<T (U::*)(BOOST_PP_ENUM_PARAMS(N, A)) const>
    {
        typedef U type;
    };

    #undef N

#endif // BOOST_PROTO_DONT_USE_PREPROCESSED_FILES
