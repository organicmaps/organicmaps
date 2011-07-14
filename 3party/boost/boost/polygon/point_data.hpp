/*
  Copyright 2008 Intel Corporation
 
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef GTLPOINT_DATA_HPP
#define GTLPOINT_DATA_HPP

#include "isotropy.hpp"

namespace boost { namespace polygon{

  template <typename T>
  class point_data {
  public:
    typedef T coordinate_type;
    inline point_data()
#ifndef BOOST_POLYGON_MSVC
      :coords_()  
#endif
    {} 
    inline point_data(coordinate_type x, coordinate_type y)
#ifndef BOOST_POLYGON_MSVC
      :coords_()  
#endif
    {
      coords_[HORIZONTAL] = x; coords_[VERTICAL] = y; 
    }
    inline point_data(const point_data& that)
#ifndef BOOST_POLYGON_MSVC
      :coords_() 
#endif
    { (*this) = that; }
    template <typename other>
    point_data(const other& that)
#ifndef BOOST_POLYGON_MSVC
      :coords_()  
#endif
    { (*this) = that; }
    inline point_data& operator=(const point_data& that) {
      coords_[0] = that.coords_[0]; coords_[1] = that.coords_[1]; return *this; 
    }
    template<typename T1, typename T2>
    inline point_data(const T1& x, const T2& y)
#ifndef BOOST_POLYGON_MSVC
      :coords_()  
#endif
    {
      coords_[HORIZONTAL] = (coordinate_type)x;
      coords_[VERTICAL] = (coordinate_type)y;
    }
    template <typename T2>
    inline point_data(const point_data<T2>& rvalue)
#ifndef BOOST_POLYGON_MSVC
      :coords_()  
#endif
    {
      coords_[HORIZONTAL] = (coordinate_type)(rvalue.x());
      coords_[VERTICAL] = (coordinate_type)(rvalue.y());
    }
    template <typename T2>
    inline point_data& operator=(const T2& rvalue);
    inline bool operator==(const point_data& that) const {
      return coords_[0] == that.coords_[0] && coords_[1] == that.coords_[1];
    }
    inline bool operator!=(const point_data& that) const {
      return !((*this) == that);
    }
    inline bool operator<(const point_data& that) const {
      return coords_[0] < that.coords_[0] ||
        (coords_[0] == that.coords_[0] && coords_[1] < that.coords_[1]);
    }
    inline bool operator<=(const point_data& that) const { return !(that < *this); }
    inline bool operator>(const point_data& that) const { return that < *this; }
    inline bool operator>=(const point_data& that) const { return !((*this) < that); }
    inline coordinate_type get(orientation_2d orient) const {
      return coords_[orient.to_int()]; 
    }
    inline void set(orientation_2d orient, coordinate_type value) {
      coords_[orient.to_int()] = value; 
    }
    inline coordinate_type x() const {
      return coords_[HORIZONTAL];
    }
    inline coordinate_type y() const {
      return coords_[VERTICAL];
    }
    inline point_data& x(coordinate_type value) {
      coords_[HORIZONTAL] = value;
      return *this;
    }
    inline point_data& y(coordinate_type value) {
      coords_[VERTICAL] = value;
      return *this;
    }
  private:
    coordinate_type coords_[2]; 
  };

}
}
#endif

