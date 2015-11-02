//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_ALLOCATOR_SCOPED_ALLOCATOR_FWD_HPP
#define BOOST_CONTAINER_ALLOCATOR_SCOPED_ALLOCATOR_FWD_HPP

//! \file
//! This header file forward declares boost::container::scoped_allocator_adaptor
//! and defines the following types:

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
#include <boost/container/detail/std_fwd.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/move/detail/fwd_macros.hpp>
#endif

namespace boost { namespace container {

#ifndef BOOST_CONTAINER_DOXYGEN_INVOKED

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

   #if !defined(BOOST_CONTAINER_UNIMPLEMENTED_PACK_EXPANSION_TO_FIXED_LIST)

      template <typename OuterAlloc, typename ...InnerAllocs>
      class scoped_allocator_adaptor;

   #else // #if !defined(BOOST_CONTAINER_UNIMPLEMENTED_PACK_EXPANSION_TO_FIXED_LIST)

      template <typename ...InnerAllocs>
      class scoped_allocator_adaptor;

      template <typename OuterAlloc, typename ...InnerAllocs>
      class scoped_allocator_adaptor<OuterAlloc, InnerAllocs...>;

   #endif   // #if !defined(BOOST_CONTAINER_UNIMPLEMENTED_PACK_EXPANSION_TO_FIXED_LIST)

#else    // #if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

   template <typename OuterAlloc, BOOST_MOVE_CLASSDFLT9>
   class scoped_allocator_adaptor;

#endif

   template <int Dummy = 0>
   struct std_allocator_arg_holder
   {
      static ::std::allocator_arg_t *dummy;
   };

   template <int Dummy>
   ::std::allocator_arg_t *std_allocator_arg_holder<Dummy>::dummy;

#else    //BOOST_CONTAINER_DOXYGEN_INVOKED

#endif   //#ifndef BOOST_CONTAINER_DOXYGEN_INVOKED

//! The allocator_arg_t struct is an empty structure type used as a unique type to
//! disambiguate constructor and function overloading. Specifically, several types
//! have constructors with allocator_arg_t as the first argument, immediately followed
//! by an argument of a type that satisfies Allocator requirements
typedef const std::allocator_arg_t & allocator_arg_t;

//! A instance of type allocator_arg_t
//!
static allocator_arg_t allocator_arg = BOOST_CONTAINER_DOC1ST(unspecified, *std_allocator_arg_holder<>::dummy);

template <class T>
struct constructible_with_allocator_suffix;

template <class T>
struct constructible_with_allocator_prefix;

template <typename T, typename Allocator>
struct uses_allocator;

}} // namespace boost { namespace container {

#include <boost/container/detail/config_end.hpp>

#endif //  BOOST_CONTAINER_ALLOCATOR_SCOPED_ALLOCATOR_FWD_HPP
