//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DETAIL_WORKAROUND_HPP
#define BOOST_CONTAINERS_DETAIL_WORKAROUND_HPP

#include "config_begin.hpp"

#if    !defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_NO_VARIADIC_TEMPLATES)\
    && !defined(BOOST_MOVE_MACRO_DISABLE_VARIADIC_TMPL)
#define BOOST_CONTAINERS_PERFECT_FORWARDING

#endif

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //#ifndef BOOST_CONTAINERS_DETAIL_WORKAROUND_HPP
