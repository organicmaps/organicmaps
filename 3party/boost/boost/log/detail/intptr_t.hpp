/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   intptr_t.hpp
 * \author Andrey Semashev
 * \date   06.05.2013
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_DETAIL_INTPTR_T_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_INTPTR_T_HPP_INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_LOG_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

// PGI seems to not support intptr_t/uintptr_t properly. BOOST_HAS_STDINT_H is not defined for this compiler by Boost.Config.
#if !defined(__PGIC__)

#if (defined(BOOST_WINDOWS) && !defined(_WIN32_WCE)) \
    || (defined(_XOPEN_UNIX) && (_XOPEN_UNIX+0 > 0) && !defined(__UCLIBC__)) \
    || defined(__CYGWIN__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__) \
    || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

typedef ::intptr_t intptr_t;
typedef ::uintptr_t uintptr_t;
#define BOOST_LOG_HAS_INTPTR_T

#elif (defined(__GNUC__) || defined(__clang__)) && defined(__INTPTR_TYPE__) && defined(__UINTPTR_TYPE__)

typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
#define BOOST_LOG_HAS_INTPTR_T

#endif

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_INTPTR_T_HPP_INCLUDED_
