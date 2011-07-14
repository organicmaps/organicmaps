/*
  Copyright 2008 Intel Corporation
 
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_INTERVAL_DATA_HPP
#define BOOST_POLYGON_INTERVAL_DATA_HPP
#include "isotropy.hpp"
namespace boost { namespace polygon{
  template <typename T>
  class interval_data {
  public:
    typedef T coordinate_type;
    inline interval_data()
#ifndef BOOST_POLYGON_MSVC 
      :coords_() 
#endif 
    {} 
    inline interval_data(coordinate_type low, coordinate_type high)
#ifndef BOOST_POLYGON_MSVC 
      :coords_() 
#endif 
    {
      coords_[LOW] = low; coords_[HIGH] = high; 
    }
    inline interval_data(const interval_data& that)
#ifndef BOOST_POLYGON_MSVC 
      :coords_() 
#endif 
    {
      (*this) = that; 
    }
    inline interval_data& operator=(const interval_data& that) {
      coords_[0] = that.coords_[0]; coords_[1] = that.coords_[1]; return *this; 
    }
    template <typename T2>
    inline interval_data& operator=(const T2& rvalue);
    inline coordinate_type get(direction_1d dir) const {
      return coords_[dir.to_int()]; 
    }
    inline coordinate_type low() const { return coords_[0]; }
    inline coordinate_type high() const { return coords_[1]; }
    inline bool operator==(const interval_data& that) const {
      return low() == that.low() && high() == that.high(); }
    inline bool operator!=(const interval_data& that) const {
      return low() != that.low() || high() != that.high(); }
    inline bool operator<(const interval_data& that) const {
      if(coords_[0] < that.coords_[0]) return true;
      if(coords_[0] > that.coords_[0]) return false;
      if(coords_[1] < that.coords_[1]) return true;
      return false;
    }
    inline bool operator<=(const interval_data& that) const { return !(that < *this); }
    inline bool operator>(const interval_data& that) const { return that < *this; }
    inline bool operator>=(const interval_data& that) const { return !((*this) < that); }
  inline void set(direction_1d dir, coordinate_type value) {
    coords_[dir.to_int()] = value; 
  }
private:
  coordinate_type coords_[2]; 
};

}
}
#endif
