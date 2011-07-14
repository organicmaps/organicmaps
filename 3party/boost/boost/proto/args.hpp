///////////////////////////////////////////////////////////////////////////////
/// \file args.hpp
/// Contains definition of \c term\<\>, \c list1\<\>, \c list2\<\>, ...
/// class templates.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_ARGS_HPP_EAN_04_01_2005
#define BOOST_PROTO_ARGS_HPP_EAN_04_01_2005

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/void.hpp>
#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        // All classes derived from std::ios_base have these public nested types,
        // and are non-copyable. This is an imperfect test, but it's the best we
        // we can do.
        template<typename T>
        yes_type check_is_iostream(
            typename T::failure *
          , typename T::Init *
          , typename T::fmtflags *
          , typename T::iostate *
          , typename T::openmode *
          , typename T::seekdir *
        );

        template<typename T>
        no_type check_is_iostream(...);

        template<typename T>
        struct is_iostream
        {
            static bool const value = sizeof(yes_type) == sizeof(check_is_iostream<T>(0,0,0,0,0,0));
            typedef mpl::bool_<value> type;
        };

        /// INTERNAL ONLY
        // This should be a customization point. And it serves the same purpose
        // as the is_noncopyable trait in Boost.Foreach. 
        template<typename T>
        struct ref_only
          : mpl::or_<
                is_function<T>
              , is_abstract<T>
              , is_iostream<T>
            >
        {};

        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits
        {
            typedef Expr value_type;
            typedef Expr &reference;
            typedef Expr const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits<Expr &>
        {
            typedef Expr value_type;
            typedef Expr &reference;
            typedef Expr &const_reference;
        };

        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits<Expr const &>
        {
            typedef Expr value_type;
            typedef Expr const &reference;
            typedef Expr const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits
        {
            typedef T value_type;
            typedef T &reference;
            typedef T const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits<T &>
        {
            typedef typename mpl::if_c<ref_only<T>::value, T &, T>::type value_type;
            typedef T &reference;
            typedef T &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits<T const &>
        {
            typedef T value_type;
            typedef T const &reference;
            typedef T const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T (&)[N]>
        {
            typedef T value_type[N];
            typedef T (&reference)[N];
            typedef T (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T const (&)[N]>
        {
            typedef T value_type[N];
            typedef T const (&reference)[N];
            typedef T const (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T[N]>
        {
            typedef T value_type[N];
            typedef T (&reference)[N];
            typedef T const (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T const[N]>
        {
            typedef T value_type[N];
            typedef T const (&reference)[N];
            typedef T const (&const_reference)[N];
        };
    }

    namespace argsns_
    {
        // This is where term and all the different listN templates are defined
        #include <boost/proto/detail/args.hpp>
    }

}}

#endif

