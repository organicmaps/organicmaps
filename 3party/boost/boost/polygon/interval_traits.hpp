/*
  Copyright 2008 Intel Corporation
 
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_INTERVAL_TRAITS_HPP
#define BOOST_POLYGON_INTERVAL_TRAITS_HPP
namespace boost { namespace polygon{
  template <typename T>
  struct interval_traits {
    typedef typename T::coordinate_type coordinate_type;

    static inline coordinate_type get(const T& interval, direction_1d dir) {
      return interval.get(dir); 
    }
  };

  template <typename T>
  struct interval_mutable_traits {
    static inline void set(T& interval, direction_1d dir, typename interval_traits<T>::coordinate_type value) {
      interval.set(dir, value); 
    }
    static inline T construct(typename interval_traits<T>::coordinate_type low_value, 
                              typename interval_traits<T>::coordinate_type high_value) {
      return T(low_value, high_value); 
    }
  };
}
}
#endif

