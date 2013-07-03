/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   explicit_operator_bool.hpp
 * \author Andrey Semashev
 * \date   08.03.2009
 *
 * This header defines a compatibility macro that implements an unspecified
 * \c bool operator idiom, which is superseded with explicit conversion operators in
 * C++0x.
 */

#ifndef BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_LOG_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_LOG_DOXYGEN_PASS) || !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)

/*!
 * \brief The macro defines an explicit operator of conversion to \c bool
 *
 * The macro should be used inside the definition of a class that has to
 * support the conversion. The class should also implement <tt>operator!</tt>,
 * in terms of which the conversion operator will be implemented.
 */
#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    BOOST_LOG_FORCEINLINE explicit operator bool () const\
    {\
        return !this->operator! ();\
    }

#elif !defined(BOOST_LOG_NO_UNSPECIFIED_BOOL)

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

#if !defined(_MSC_VER)

    struct unspecified_bool
    {
        // NOTE TO THE USER: If you see this in error messages then you tried
        // to apply an unsupported operator on the object that supports
        // explicit conversion to bool.
        struct OPERATORS_NOT_ALLOWED;
        static void true_value(OPERATORS_NOT_ALLOWED*) {}
    };
    typedef void (*unspecified_bool_type)(unspecified_bool::OPERATORS_NOT_ALLOWED*);

#else

    // MSVC is too eager to convert pointer to function to void* even though it shouldn't
    struct unspecified_bool
    {
        // NOTE TO THE USER: If you see this in error messages then you tried
        // to apply an unsupported operator on the object that supports
        // explicit conversion to bool.
        struct OPERATORS_NOT_ALLOWED;
        void true_value(OPERATORS_NOT_ALLOWED*) {}
    };
    typedef void (unspecified_bool::*unspecified_bool_type)(unspecified_bool::OPERATORS_NOT_ALLOWED*);

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    BOOST_LOG_FORCEINLINE operator boost::log::aux::unspecified_bool_type () const\
    {\
        if (!this->operator!())\
            return &boost::log::aux::unspecified_bool::true_value;\
        else\
            return 0;\
    }

#else

#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    BOOST_LOG_FORCEINLINE operator bool () const\
    {\
        return !this->operator! ();\
    }

#endif

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_
