//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_DETAIL_IS_SAME_HPP
#define BOOST_GRAPH_DETAIL_IS_SAME_HPP

// Deprecate the use of this header.
// TODO: Remove this file from trunk/release in 1.41/1.42.
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__)
#  pragma message ("Warning: This header is deprecated. Please use: boost/type_traits/is_same.hpp")
#elif defined(__GNUC__) || defined(__HP_aCC) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#  warning "This header is deprecated. Please use: boost/type_traits/is_same.hpp"
#endif

#include <boost/mpl/if.hpp>

namespace boost {
  struct false_tag;
  struct true_tag;

  namespace graph_detail {
    
#if !defined BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    template <class U, class V>
    struct is_same {
      typedef boost::false_tag is_same_tag; 
    };
    template <class U>
    struct is_same<U, U> {
      typedef boost::true_tag is_same_tag;
    };
#else
    template <class U, class V>
    struct is_same {
      enum { Unum = U::num, Vnum = V::num };
      typedef typename mpl::if_c< (Unum == Vnum),
               boost::true_tag, boost::false_tag>::type is_same_tag;
    };
#endif
  } // namespace graph_detail
} // namespace boost

#endif
