/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_SEGMENT_TRAITS_HPP
#define BOOST_POLYGON_SEGMENT_TRAITS_HPP
namespace boost {
namespace polygon {
  template <typename Segment>
  struct segment_traits {
    typedef typename Segment::coordinate_type coordinate_type;
    typedef typename Segment::point_type point_type;

    static inline point_type get(const Segment& segment, direction_1d dir) {
      return segment.get(dir);
    }
  };

  template <typename Segment>
  struct segment_mutable_traits {
    typedef typename segment_traits<Segment>::point_type point_type;

    static inline void set(
        Segment& segment, direction_1d dir, const point_type& point) {
      segment.set(dir, point);
    }

    static inline Segment construct(
        const point_type& low, const point_type& high) {
      return Segment(low, high);
    }
  };
}
}
#endif
