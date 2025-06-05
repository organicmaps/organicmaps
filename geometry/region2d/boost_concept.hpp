#pragma once

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <boost/polygon/detail/polygon_sort_adaptor.hpp>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-std-move"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <boost/polygon/polygon.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vector>

namespace boost
{
namespace polygon
{
typedef int32_t my_coord_t;

////////////////////////////////////////////////////////////////////////////////
// Point concept.
////////////////////////////////////////////////////////////////////////////////
typedef m2::PointI my_point_t;

template <>
struct geometry_concept<my_point_t>
{
  typedef point_concept type;
};

template <>
struct point_traits<my_point_t>
{
  typedef my_point_t::value_type coordinate_type;

  static inline coordinate_type get(my_point_t const & p, orientation_2d o) { return ((o == HORIZONTAL) ? p.x : p.y); }
};

template <>
struct point_mutable_traits<my_point_t>
{
  typedef my_point_t::value_type Coord;

  static inline void set(my_point_t & p, orientation_2d o, Coord v)
  {
    if (o == HORIZONTAL)
      p.x = v;
    else
      p.y = v;
  }

  static inline my_point_t construct(Coord x, Coord y) { return my_point_t(x, y); }
};

////////////////////////////////////////////////////////////////////////////////
// Polygon concept.
////////////////////////////////////////////////////////////////////////////////
typedef m2::RegionI my_region_t;

template <>
struct geometry_concept<my_region_t>
{
  typedef polygon_concept type;
};

template <>
struct polygon_traits<my_region_t>
{
  typedef my_region_t::Coord coordinate_type;
  typedef my_region_t::IteratorT iterator_type;
  typedef my_region_t::Value point_type;

  // Get the begin iterator
  static inline iterator_type begin_points(my_region_t const & t) { return t.Begin(); }

  // Get the end iterator
  static inline iterator_type end_points(my_region_t const & t) { return t.End(); }

  // Get the number of sides of the polygon
  static inline size_t size(my_region_t const & t) { return t.Size(); }

  // Get the winding direction of the polygon
  static inline winding_direction winding(my_region_t const & /*t*/) { return unknown_winding; }
};

struct my_point_getter
{
  my_point_t operator()(point_data<my_coord_t> const & t) { return my_point_t(t.x(), t.y()); }
};

template <>
struct polygon_mutable_traits<my_region_t>
{
  // expects stl style iterators
  template <typename IterT>
  static inline my_region_t & set_points(my_region_t & t, IterT b, IterT e)
  {
    t.AssignEx(b, e, my_point_getter());
    return t;
  }
};

////////////////////////////////////////////////////////////////////////////////
// Polygon set concept.
////////////////////////////////////////////////////////////////////////////////
typedef std::vector<my_region_t> my_region_set_t;

template <>
struct geometry_concept<my_region_set_t>
{
  typedef polygon_set_concept type;
};

// next we map to the concept through traits
template <>
struct polygon_set_traits<my_region_set_t>
{
  typedef my_coord_t coordinate_type;
  typedef my_region_set_t::const_iterator iterator_type;
  typedef my_region_set_t operator_arg_type;

  static inline iterator_type begin(my_region_set_t const & t) { return t.begin(); }

  static inline iterator_type end(my_region_set_t const & t) { return t.end(); }

  // don't worry about these, just return false from them
  static inline bool clean(my_region_set_t const & /*t*/) { return false; }
  static inline bool sorted(my_region_set_t const & /*t*/) { return false; }
};

template <>
struct polygon_set_mutable_traits<my_region_set_t>
{
  template <typename IterT>
  static inline void set(my_region_set_t & poly_set, IterT b, IterT e)
  {
    poly_set.clear();

    // this is kind of cheesy. I am copying the unknown input geometry
    // into my own polygon set and then calling get to populate the std::vector
    polygon_set_data<my_coord_t> ps;
    ps.insert(b, e);
    ps.get(poly_set);

    // if you had your own odd-ball polygon set you would probably have
    // to iterate through each polygon at this point and do something extra
  }
};
}  // namespace polygon
}  // namespace boost
