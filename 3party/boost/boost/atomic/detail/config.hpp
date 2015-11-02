/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2012 Hartmut Kaiser
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/config.hpp
 *
 * This header defines configuraion macros for Boost.Atomic
 */

#ifndef BOOST_ATOMIC_DETAIL_CONFIG_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CONFIG_HPP_INCLUDED_

#include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__CUDACC__)
// nvcc does not support alternatives in asm statement constraints
#define BOOST_ATOMIC_DETAIL_NO_ASM_CONSTRAINT_ALTERNATIVES
// nvcc does not support condition code register ("cc") clobber in asm statements
#define BOOST_ATOMIC_DETAIL_NO_ASM_CLOBBER_CC
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_ASM_CLOBBER_CC)
#define BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC "cc"
#define BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA "cc",
#else
#define BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC
#define BOOST_ATOMIC_DETAIL_ASM_CLOBBER_CC_COMMA
#endif

#endif // BOOST_ATOMIC_DETAIL_CONFIG_HPP_INCLUDED_
