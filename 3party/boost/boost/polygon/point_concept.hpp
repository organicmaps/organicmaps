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
    typedef typename coordinate_traits<typename point_traits<T>::coordinate_type>::coordinate_difference type; };

  template <typename T>
  struct point_difference_type {
      typedef typename point_difference_type_by_concept<
            T, typename is_point_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct point_distance_type_by_concept { typedef void type; };
  template <typename T>
  struct point_distance_type_by_concept<T, gtl_yes> { 
    typedef typename coordinate_traits<typename point_traits<T>::coordinate_type>::coordinate_distance type; };

  template <typename T>
  struct point_distance_type {
      typedef typename point_distance_type_by_concept<
            T, typename is_point_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T>
  typename point_coordinate_type<T>::type 
  get(const T& point, orientation_2d orient,
  typename enable_if< typename gtl_if<typename is_point_concept<typename geometry_concept<T>::type>::type>::type>::type * = 0
  ) {
    return point_traits<T>::get(point, orient);
  }
  
  template <typename T, typename coordinate_type>
  void 
  set(T& point, orientation_2d orient, coordinate_type value,
  typename enable_if<typename is_mutable_point_concept<typename geometry_concept<T>::type>::type>::type * = 0
  ) {
    point_mutable_traits<T>::set(point, orient, value);
  }
  
  template <typename T, typename coordinate_type1, typename coordinate_type2>
  T
  construct(coordinate_type1 x_value, coordinate_type2 y_value,
  typename enable_if<typename is_mutable_point_concept<typename geometry_concept<T>::type>::type>::type * = 0
  ) {
    return point_mutable_traits<T>::construct(x_value, y_value); 
  }

  template <typename T1, typename T2>
  T1&
  assign(T1& lvalue, const T2& rvalue,
  typename enable_if< typename gtl_and< typename is_mutable_point_concept<typename geometry_concept<T1>::type>::type,
                                         typename is_point_concept<typename geometry_concept<T2>::type>::type>::type>::type * = 0
  ) {
    set(lvalue, HORIZONTAL, get(rvalue, HORIZONTAL));
    set(lvalue, VERTICAL, get(rvalue, VERTICAL));
    return lvalue;
  }

  struct y_p_x : gtl_yes {};

  template <typename point_type>
  typename enable_if< typename gtl_and<y_p_x, typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type, 
                       typename point_traits<point_type>::coordinate_type >::type 
  x(const point_type& point) {
    return get(point, HORIZONTAL);
  }

  struct y_p_y : gtl_yes {};

  template <typename point_type>
  typename enable_if< typename gtl_and<y_p_y, typename is_point_concept<typename geometry_concept<point_type>::type>::type>::type, 
                       typename point_traits<point_type>::coordinate_type >::type 
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

  template <typename T, typename T2>
  bool
  equivalence(const T& point1, const T2& point2,
    typename enable_if< typename gtl_and<typename gtl_same_type<point_concept, typename geometry_concept<T>::type>::type,
              typename is_point_concept<typename geometry_concept<T2>::type>::type>::type>::type * = 0
  ) {
    typename point_traits<T>::coordinate_type x1 = x(point1);
    typename point_traits<T2>::coordinate_type x2 = get(point2, HORIZONTAL);
    typename point_traits<T>::coordinate_type y1 = get(point1, VERTICAL);
    typename point_traits<T2>::coordinate_type y2 = y(point2);
    return x1 == x2 && y1 == y2;
  }

  template <typename point_type_1, typename point_type_2>
  typename point_difference_type<point_type_1>::type
  manhattan_distance(const point_type_1& point1, const point_type_2& point2,
  typename enable_if< typename gtl_and<typename gtl_same_type<point_concept, typename geometry_concept<point_type_1>::type>::type, 
  typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type>::type * = 0) {
    return euclidean_distance(point1, point2, HORIZONTAL) + euclidean_distance(point1, point2, VERTICAL);
  }
  
  struct y_i_ed1 : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<y_i_ed1, typename is_point_concept<typename geometry_concept<point_type_1>::type>::type, 
  typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type,
  typename point_difference_type<point_type_1>::type>::type
  euclidean_distance(const point_type_1& point1, const point_type_2& point2, orientation_2d orient) {
    typename coordinate_traits<typename point_traits<point_type_1>::coordinate_type>::coordinate_difference return_value =
      get(point1, orient) - get(point2, orient);
    return return_value < 0 ? (typename coordinate_traits<typename point_traits<point_type_1>::coordinate_type>::coordinate_difference)-return_value : return_value;
  }
  
  struct y_i_ed2 : gtl_yes {};

  template <typename point_type_1, typename point_type_2>
  typename enable_if< typename gtl_and_3<y_i_ed2, typename gtl_same_type<point_concept, typename geometry_concept<point_type_1>::type>::type,
  typename gtl_same_type<point_concept, typename geometry_concept<point_type_2>::type>::type>::type,
  typename point_distance_type<point_type_1>::type>::type
  euclidean_distance(const point_type_1& point1, const point_type_2& point2) {
    typedef typename point_traits<point_type_1>::coordinate_type Unit;
    return sqrt((double)(distance_squared(point1, point2)));
  }
  
  template <typename point_type_1, typename point_type_2>
  typename point_difference_type<point_type_1>::type
  distance_squared(const point_type_1& point1, const point_type_2& point2,
    typename enable_if< typename gtl_and<typename is_point_concept<typename geometry_concept<point_type_1>::type>::type,
                   typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type>::type * = 0
  ) {
    typedef typename point_traits<point_type_1>::coordinate_type Unit;
    typename coordinate_traits<Unit>::coordinate_difference dx = euclidean_distance(point1, point2, HORIZONTAL);
    typename coordinate_traits<Unit>::coordinate_difference dy = euclidean_distance(point1, point2, VERTICAL);
    dx *= dx;
    dy *= dy;
    return dx + dy;
  }

  template <typename point_type_1, typename point_type_2>
  point_type_1 &
  convolve(point_type_1& lvalue, const point_type_2& rvalue,
  typename enable_if< typename gtl_and<typename is_mutable_point_concept<typename geometry_concept<point_type_1>::type>::type, 
  typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type>::type * = 0
  ) {
    x(lvalue, x(lvalue) + x(rvalue));
    y(lvalue, y(lvalue) + y(rvalue));
    return lvalue;
  }
  
  template <typename point_type_1, typename point_type_2>
  point_type_1 &
  deconvolve(point_type_1& lvalue, const point_type_2& rvalue,
  typename enable_if< typename gtl_and<typename is_mutable_point_concept<typename geometry_concept<point_type_1>::type>::type, 
  typename is_point_concept<typename geometry_concept<point_type_2>::type>::type>::type>::type * = 0
  ) {
    x(lvalue, x(lvalue) - x(rvalue));
    y(lvalue, y(lvalue) - y(rvalue));
    return lvalue;
  }
  
  template <typename point_type, typename coord_type>
  point_type &
  scale_up(point_type& point, coord_type factor,
  typename enable_if<typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type * = 0
  ) {
    typedef typename point_traits<point_type>::coordinate_type Unit;
    x(point, x(point) * (Unit)factor);
    y(point, y(point) * (Unit)factor);
    return point;
  }

  template <typename point_type, typename coord_type>
  point_type &
  scale_down(point_type& point, coord_type factor,
  typename enable_if<typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type * = 0
  ) {
    typedef typename point_traits<point_type>::coordinate_type Unit;
    typedef typename coordinate_traits<Unit>::coordinate_distance dt;
    x(point, scaling_policy<Unit>::round((dt)((dt)(x(point)) / (dt)factor))); 
    y(point, scaling_policy<Unit>::round((dt)((dt)(y(point)) / (dt)factor))); 
    return point;
  }

  template <typename point_type, typename scaling_type>
  point_type &
  scale(point_type& point, 
        const scaling_type& scaling,
        typename enable_if<typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type * = 0
        ) {
    typedef typename point_traits<point_type>::coordinate_type Unit;
    Unit x_(x(point)), y_(y(point));
    scaling.scale(x_, y_);
    x(point, x_);
    y(point, y_);
    return point;
  }

  template <typename point_type, typename transformation_type>
  point_type &
  transform(point_type& point, const transformation_type& transformation,
  typename enable_if<typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type * = 0
  ) {
    typedef typename point_traits<point_type>::coordinate_type Unit;
    Unit x_(x(point)), y_(y(point));
    transformation.transform(x_, y_);
    x(point, x_);
    y(point, y_);
    return point;
  }

  struct y_pt_move : gtl_yes {};

  template <typename point_type>
  typename enable_if< 
    typename gtl_and< y_pt_move, 
                      typename is_mutable_point_concept<
      typename geometry_concept<point_type>::type>::type>::type, 
    point_type>::type &
  move(point_type& point, orientation_2d orient,
       typename point_traits<point_type>::coordinate_type displacement,
       typename enable_if<typename is_mutable_point_concept<typename geometry_concept<point_type>::type>::type>::type * = 0
       ) {
    typedef typename point_traits<point_type>::coordinate_type Unit;
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

