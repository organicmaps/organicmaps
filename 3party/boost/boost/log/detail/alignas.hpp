/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   alignas.hpp
 * \author Andrey Semashev
 * \date   06.07.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_DETAIL_ALIGNAS_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ALIGNAS_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_LOG_HAS_PRAGMA_ONCE
#pragma once
#endif

// The macro allows to specify type or variable alignment
#if defined(_MSC_VER)
#define BOOST_LOG_ALIGNAS(x) __declspec(align(x))
#elif defined(BOOST_CLANG)
#if __has_feature(cxx_alignas) || __has_extension(cxx_alignas)
#define BOOST_LOG_ALIGNAS(x) alignas(x)
#endif
#elif defined(__GNUC__)
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#define BOOST_LOG_ALIGNAS(x) alignas(x)
#else
#define BOOST_LOG_ALIGNAS(x) __attribute__((__aligned__(x)))
#endif
#endif

#if !defined(BOOST_LOG_ALIGNAS)
#define BOOST_LOG_NO_ALIGNAS 1
#define BOOST_LOG_ALIGNAS(x)
#endif

#endif // BOOST_LOG_DETAIL_ALIGNAS_HPP_INCLUDED_
