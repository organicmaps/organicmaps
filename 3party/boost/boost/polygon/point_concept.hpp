/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POINT_CONCEPT_HPP
#define BOOST_POLYGON_POINT_CONCEPT_HPP
#include "isotropy.hpp"
#include "point_data.hpp"
#include "point_traits.hpp"

namespace boost { namespace polygon{
  struct point_concept {};

  template <typename T>
  struct is_point_concept { typedef gtl_no type; };
  template <>
  struct is_point_concept<point_concept> { typedef gtl_yes type; };

  struct point_3d_concept;
  template <>
  struct is_point_concept<point_3d_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_mutable_point_concept { typedef gtl_no type; };
  template <>
  struct is_mutable_point_concept<point_concept> { typedef gtl_yes type; };

  template <typename T, typename CT>
  struct point_coordinate_type_by_concept { typedef void type; };
  template <typename T>
  struct point_coordinate_type_by_concept<T, gtl_yes> { typedef typename point_traits<T>::coordinate_type type; };

  template <typename T>
  struct point_coordinate_type {
      typedef typename point_coordinate_type_by_concept<T, typename is_point_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct point_difference_type_by_concept { typedef void type; };
  template <typename T>
  struct point_difference_type_by_concept<T, gtl_yes> {
    typedef typename coordinate_traits<typename point_coordinate_type<T>::type>::coordinate_difference type; };

  template <typename T>
  struct point_difference_type {
      typedef typename point_difference_type_by_concept<
            T, typename is_point_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct point_distance_type_by_concept { typedef void type; };
  template <typename T>
  struct point_distance_type_by_concept<T, gtl_yes> {
    typedef typename coordinate_traits<typename point_coordinate_type<T>::type>::coordinate_distance type; };

  template <typename T>
  struct point_distance_type {
      typedef typename point_distance_type_by_concept<
            T, typename is_point_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  struct y_pt_get : gtl_yes {};

  template <typename T>
  typename enable_if< typename gtl_and<y_pt_get, typename is_point_concept<typename geometry_concept<T>::type>::type>::type,
                      typename point_coordinate_type<T>::type >::type
  get(const T& point, orientation_2d orient) {
    return point_traits<T>::get(point, orient);
  }

  struct y_pt_set : gtl_yes {};

  template <typename T, typename coordinate_type>
  typename enable_if< typename gtl_and<y_pt_set, typename is_mutable_point_concept<typename geometry_concept<T>::type>::type>::type,
                      void>::type
  set(T& point, orientation_2d orient, coordinate_type value) {
    point_mutable_traits<T>::set(point, orient, value);
  }

  struct y_pt_construct : gtl_yes {};

  template <typename T, typename coordinate_type1, typename coordinate_type2>
  typename enable_if< typename gtl_and<y_pt_construct, typename is_mutable_point_concept<typename geometry_concept<T>::type>::type>::type,
                      T>::type
  construct(coordinate_type1 x_value, coordinate_type2 y_value) {
    return point_mutable_traits<T>::construct(x_value, y_value);
  }

  struct y_pt_assign : gtl_yes {};

  template <typename T1, typename T2>
  typename enable_if<typename gtl_and_3<
        y_pt_assign,
        typename is_mutable_point_concept<typename geometry_concept<T1>::type>::type,
        typename is_point_concept<typename geometry_concept<T2>::type>::type>::type,
      T1>::type &
  assign(T1& lvalue, const T2& rvalue) {
    set(lvalue, HORIZONTAL, get(rvalue, HORIZONTAL));
    set(lvalue, VERTICAL, get(rvalue, VERTICAL));
    return lvalue;
  }

  struct y_p_x : gtl_yes {};

  template <typename point_type>
  typename enable_if< typename gtl_and<y_p_x, typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      typename point_coordinate_type<point_type>::type >::type
  x(const point_type& point) {
    return get(point, HORIZONTAL);
  }

  struct y_p_y : gtl_yes {};

  template <typename point_type>
  typename enable_if< typename gtl_and<y_p_y, typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      typename point_coordinate_type<point_type>::type >::type
  y(const point_type& point) {
    return get(point, VERTICAL);
  }

  struct y_p_sx : gtl_yes {};

  template <typename point_type, typename coordinate_type>
  typename enable_if<typename gtl_and<y_p_sx, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      void>::type
  x(point_type& point, coordinate_type value) {
    set(point, HORIZONTAL, value);
  }

  struct y_p_sy : gtl_yes {};

  template <typename point_type, typename coordinate_type>
  typename enable_if<typename gtl_and<y_p_sy, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      void>::type
  y(point_type& point, coordinate_type value) {
    set(point, VERTICAL, value);
  }

  struct y_pt_equiv : gtl_yes {};

  template <typename T, typename T2>
  typename enable_if<typename gtl_and_3<y_pt_equiv,
        typename gtl_same_type<point_concept, typename geometry_concept<T>::type>::type,
        typename is_point_concept<typename geometry_concept<T2>::type>::type>::type,
      bool>::type
  equivalence(const T& point1, const T2& point2) {
    typename point_coordinate_type<T>::type x1 = x(point1);
    typename point_coordinate_type<T2>::type x2 = get(point2, HORIZONTAL);
    typename point_coordinate_type<T>::type y1 = get(point1, VERTICAL);
    typename point_coordinate_type<T2>::type y2 = y(point2);
    return x1 == x2 && y1 == y2;
  }

  struct y_pt_man_dist : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<
        y_pt_man_dist,
        typename gtl_same_type<point_concept, typename geometry_concept<point_type_1>::type>::type,
        typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
      typename point_difference_type<point_type_1>::type>::type
  manhattan_distance(const point_type_1& point1, const point_type_2& point2) {
    return euclidean_distance(point1, point2, HORIZONTAL) + euclidean_distance(point1, point2, VERTICAL);
  }

  struct y_pt_ed1 : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<y_pt_ed1, typename is_point_concept<typename geometry_concept<point_type_1>::type>::type,
  typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
  typename point_difference_type<point_type_1>::type>::type
  euclidean_distance(const point_type_1& point1, const point_type_2& point2, orientation_2d orient) {
    typename coordinate_traits<typename point_coordinate_type<point_type_1>::type>::coordinate_difference return_value =
      get(point1, orient) - get(point2, orient);
    return return_value < 0 ? (typename coordinate_traits<typename point_coordinate_type<point_type_1>::type>::coordinate_difference)-return_value : return_value;
  }

  struct y_pt_ed2 : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<y_pt_ed2, typename gtl_same_type<point_concept, typename geometry_concept<point_type_1>::type>::type,
  typename gtl_same_type<point_concept, typename geometry_concept<point_type_2>::type>::type>::type,
  typename point_distance_type<point_type_1>::type>::type
  euclidean_distance(const point_type_1& point1, const point_type_2& point2) {
    typedef typename point_coordinate_type<point_type_1>::type Unit;
    return std::sqrt((double)(distance_squared(point1, point2)));
  }

  struct y_pt_eds : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<
        y_pt_eds,
        typename is_point_concept<typename geometry_concept<point_type_1>::type>::type,
        typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
      typename point_difference_type<point_type_1>::type>::type
  distance_squared(const point_type_1& point1, const point_type_2& point2) {
    typedef typename point_coordinate_type<point_type_1>::type Unit;
    typename coordinate_traits<Unit>::coordinate_difference dx = euclidean_distance(point1, point2, HORIZONTAL);
    typename coordinate_traits<Unit>::coordinate_difference dy = euclidean_distance(point1, point2, VERTICAL);
    dx *= dx;
    dy *= dy;
    return dx + dy;
  }

  struct y_pt_convolve : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<
        y_pt_convolve,
        typename is_mutable_point_concept<typename geometry_concept<point_type_1>::type>::type,
        typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
      point_type_1>::type &
  convolve(point_type_1& lvalue, const point_type_2& rvalue) {
    x(lvalue, x(lvalue) + x(rvalue));
    y(lvalue, y(lvalue) + y(rvalue));
    return lvalue;
  }

  struct y_pt_deconvolve : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<
        y_pt_deconvolve,
        typename is_mutable_point_concept<typename geometry_concept<point_type_1>::type>::type,
        typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
      point_type_1>::type &
  deconvolve(point_type_1& lvalue, const point_type_2& rvalue) {
    x(lvalue, x(lvalue) - x(rvalue));
    y(lvalue, y(lvalue) - y(rvalue));
    return lvalue;
  }

  struct y_pt_scale_up : gtl_yes {};

  template <typename point_type, typename coord_type>
  typename enable_if< typename gtl_and<y_pt_scale_up, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      point_type>::type &
  scale_up(point_type& point, coord_type factor) {
    typedef typename point_coordinate_type<point_type>::type Unit;
    x(point, x(point) * (Unit)factor);
    y(point, y(point) * (Unit)factor);
    return point;
  }

  struct y_pt_scale_down : gtl_yes {};

  template <typename point_type, typename coord_type>
  typename enable_if< typename gtl_and<y_pt_scale_down, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      point_type>::type &
  scale_down(point_type& point, coord_type factor) {
    typedef typename point_coordinate_type<point_type>::type Unit;
    typedef typename coordinate_traits<Unit>::coordinate_distance dt;
    x(point, scaling_policy<Unit>::round((dt)((dt)(x(point)) / (dt)factor)));
    y(point, scaling_policy<Unit>::round((dt)((dt)(y(point)) / (dt)factor)));
    return point;
  }

  struct y_pt_scale : gtl_yes {};

  template <typename point_type, typename scaling_type>
  typename enable_if< typename gtl_and<y_pt_scale, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      point_type>::type &
  scale(point_type& point, const scaling_type& scaling) {
    typedef typename point_coordinate_type<point_type>::type Unit;
    Unit x_(x(point)), y_(y(point));
    scaling.scale(x_, y_);
    x(point, x_);
    y(point, y_);
    return point;
  }

  struct y_pt_transform : gtl_yes {};

  template <typename point_type, typename transformation_type>
  typename enable_if< typename gtl_and<y_pt_transform, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      point_type>::type &
  transform(point_type& point, const transformation_type& transformation) {
    typedef typename point_coordinate_type<point_type>::type Unit;
    Unit x_(x(point)), y_(y(point));
    transformation.transform(x_, y_);
    x(point, x_);
    y(point, y_);
    return point;
  }

  struct y_pt_move : gtl_yes {};

  template <typename point_type>
  typename enable_if< typename gtl_and<y_pt_move, typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type,
                      point_type>::type &
  move(point_type& point, orientation_2d orient,
       typename point_coordinate_type<point_type>::type displacement) {
    typedef typename point_coordinate_type<point_type>::type Unit;
    Unit v(get(point, orient));
    set(point, orient, v + displacement);
    return point;
  }

  template <class T>
  template <class T2>
  point_data<T>& point_data<T>::operator=(const T2& rvalue) {
    assign(*this, rvalue);
    return *this;
  }

  template <typename T>
  struct geometry_concept<point_data<T> > {
    typedef point_concept type;
  };
}
}
#endif
