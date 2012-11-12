/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_SEGMENT_DATA_HPP
#define BOOST_POLYGON_SEGMENT_DATA_HPP

#include "isotropy.hpp"

namespace boost {
namespace polygon {
template <typename T>
class segment_data {
 public:
  typedef T coordinate_type;
  typedef point_data<T> point_type;

  inline segment_data()
#ifndef BOOST_POLYGON_MSVC
    :points_()
#endif
  {}

  inline segment_data(const point_type& low, const point_type& high)
#ifndef BOOST_POLYGON_MSVC
    :points_()
#endif
  {
    points_[LOW] = low;
    points_[HIGH] = high;
  }

  inline segment_data(const segment_data& that)
#ifndef BOOST_POLYGON_MSVC
    :points_()
#endif
  {
    (*this) = that;
  }

  inline segment_data& operator=(const segment_data& that) {
    points_[0] = that.points_[0];
    points_[1] = that.points_[1];
    return *this;
  }

  template <typename Segment>
  inline segment_data& operator=(const Segment& that);

  inline point_type get(direction_1d dir) const {
    return points_[dir.to_int()];
  }

  inline void set(direction_1d dir, const point_type& point) {
    points_[dir.to_int()] = point;
  }

  inline point_type low() const { return points_[0]; }

  inline segment_data& low(const point_type& point) {
    points_[0] = point;
    return *this;
  }

  inline point_type high() const {return points_[1]; }

  inline segment_data& high(const point_type& point) {
    points_[1] = point;
    return *this;
  }

  inline bool operator==(const segment_data& that) const {
    return low() == that.low() && high() == that.high();
  }

  inline bool operator!=(const segment_data& that) const {
    return low() != that.low() || high() != that.high();
  }

  inline bool operator<(const segment_data& that) const {
    if (points_[0] < that.points_[0])
      return true;
    if (points_[0] > that.points_[0])
      return false;
    return points_[1] < that.points_[1];
  }

  inline bool operator<=(const segment_data& that) const {
    return !(that < *this);
  }

  inline bool operator>(const segment_data& that) const {
    return that < *this;
  }

  inline bool operator>=(const segment_data& that) const {
    return !((*this) < that);
  }

 private:
  point_type points_[2];
};
}
}
#endif
