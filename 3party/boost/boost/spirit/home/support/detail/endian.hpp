//  Copyright (c) 2001-2011 Hartmut Kaiser
//  http://spirit.sourceforge.net/
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_ENDIAN_MAR_21_2009_0349PM)
#define SPIRIT_ENDIAN_MAR_21_2009_0349PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/version.hpp>

// We need to treat the endian number types as PODs
#define BOOST_ENDIAN_FORCE_PODNESS

// If Boost has the endian library, use it, otherwise use an adapted version 
// included with Spirit
#if BOOST_VERSION >= 104800
#include <boost/integer/endian.hpp>
#else
#include <boost/spirit/home/support/detail/integer/endian.hpp>
#endif

#endif
