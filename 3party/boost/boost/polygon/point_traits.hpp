/*
  Copyright 2008 Intel Corporation
 
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POINT_TRAITS_HPP
#define BOOST_POLYGON_POINT_TRAITS_HPP

#include "isotropy.hpp"

namespace boost { namespace polygon{
  template <typename T>
  struct point_traits {
    typedef typename T::coordinate_type coordinate_type;
  
    static inline coordinate_type get(const T& point, orientation_2d orient) {
      return point.get(orient); 
    }
  };

  template <typename T>
  struct point_mutable_traits {
    static inline void set(T& point, orientation_2d orient, typename point_traits<T>::coordinate_type value) {
      point.set(orient, value); 
    }
    static inline T construct(typename point_traits<T>::coordinate_type x_value, typename point_traits<T>::coordinate_type y_value) {
      return T(x_value, y_value); 
    }
  };
}
}
#endif

