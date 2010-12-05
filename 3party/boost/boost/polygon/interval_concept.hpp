/*
  Copyright 2008 Intel Corporation
 
  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_INTERVAL_CONCEPT_HPP
#define BOOST_POLYGON_INTERVAL_CONCEPT_HPP
#include "isotropy.hpp"
#include "interval_data.hpp"
#include "interval_traits.hpp"

namespace boost { namespace polygon{
  struct interval_concept {};
 
  template <typename T>
  struct is_interval_concept { typedef gtl_no type; };
  template <>
  struct is_interval_concept<interval_concept> { typedef gtl_yes type; };

  template <typename T>
  struct is_mutable_interval_concept { typedef gtl_no type; };
  template <>
  struct is_mutable_interval_concept<interval_concept> { typedef gtl_yes type; };

  template <typename T, typename CT>
  struct interval_coordinate_type_by_concept { typedef void type; };
  template <typename T>
  struct interval_coordinate_type_by_concept<T, gtl_yes> { typedef typename interval_traits<T>::coordinate_type type; };

  template <typename T>
  struct interval_coordinate_type {
      typedef typename interval_coordinate_type_by_concept<
            T, typename is_interval_concept<typename geometry_concept<T>::type>::type>::type type;
  };

  template <typename T, typename CT>
  struct interval_difference_type_by_concept { typedef void type; };
  template <typename T>
  struct interval_difference_type_by_concept<T, gtl_yes> { 
    typedef typename coordinate_traits<typename interval_traits<T>::coordinate_type>::coordinate_difference type; };

  template <typename T>
  struct interval_difference_type {
      typedef typename interval_difference_type_by_concept<
            T, typename is_interval_concept<typename geometry_concept<T>::type>::type>::type type;
  };


  template <typename T>
  typename interval_coordinate_type<T>::type
  get(const T& interval, direction_1d dir,
  typename enable_if<typename gtl_if<typename is_interval_concept<typename geometry_concept<T>::type>::type>::type>::type * = 0
  ) {
    return interval_traits<T>::get(interval, dir); 
  }

  template <typename T, typename coordinate_type>
  void 
  set(T& interval, direction_1d dir, coordinate_type value,
  typename enable_if<typename is_mutable_interval_concept<typename geometry_concept<T>::type>::type>::type * = 0
  ) {
    //this may need to be refined
    interval_mutable_traits<T>::set(interval, dir, value); 
    if(high(interval) < low(interval))
      interval_mutable_traits<T>::set(interval, dir.backward(), value);
  }
  
  template <typename T, typename T2, typename T3>
  T
  construct(T2 low_value, T3 high_value,
            typename enable_if<typename is_mutable_interval_concept<typename geometry_concept<T>::type>::type>::type * = 0
  ) {
    if(low_value > high_value) std::swap(low_value, high_value);
    return interval_mutable_traits<T>::construct(low_value, high_value); 
  }
  
  template <typename T, typename T2>
  T
  copy_construct(const T2& interval,
  typename enable_if< typename gtl_and<typename is_mutable_interval_concept<typename geometry_concept<T>::type>::type,
  typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type>::type * = 0
  ) {
    return construct<T>
      (get(interval, LOW ),
       get(interval, HIGH));
  }

  template <typename T1, typename T2>
  T1 &
  assign(T1& lvalue, const T2& rvalue,
  typename enable_if< typename gtl_and< typename is_mutable_interval_concept<typename geometry_concept<T1>::type>::type,
  typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type>::type * = 0) {
    lvalue = copy_construct<T1>(rvalue);
    return lvalue;
  }

  template <typename T, typename T2>
  bool 
  equivalence(const T& interval1, const T2& interval2,
  typename enable_if< typename gtl_and< typename is_interval_concept<typename geometry_concept<T>::type>::type,
  typename is_interval_concept<typename geometry_concept<T2>::type>::type>::type>::type * = 0
  ) {
    return get(interval1, LOW) ==
      get(interval2, LOW) &&
      get(interval1, HIGH) ==
      get(interval2, HIGH); 
  }
  
  struct y_i_contains : gtl_yes {};

  template <typename interval_type>
  typename enable_if< typename gtl_and< y_i_contains, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type >::type, bool>::type 
  contains(const interval_type& interval,
           typename interval_traits<interval_type>::coordinate_type value, 
           bool consider_touch = true ) {
    if(consider_touch) {
      return value <= high(interval) && value >= low(interval);
    } else {
      return value < high(interval) && value > low(interval);
    }
  }
  
  template <typename interval_type, typename interval_type_2>
  bool 
  contains(const interval_type& interval,
           const interval_type_2& value, bool consider_touch = true,
           typename enable_if< typename gtl_and< typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
           typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type>::type * = 0
           ) {
    return contains(interval, get(value, LOW), consider_touch) &&
      contains(interval, get(value, HIGH), consider_touch);
  }
  
  // get the low coordinate
  template <typename interval_type>
  typename interval_traits<interval_type>::coordinate_type 
  low(const interval_type& interval,
  typename enable_if< typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type * = 0
  ) { return get(interval, LOW); }

  // get the high coordinate
  template <typename interval_type>
  typename interval_traits<interval_type>::coordinate_type 
  high(const interval_type& interval,
  typename enable_if< typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type * = 0
  ) { return get(interval, HIGH); }

  // get the center coordinate
  template <typename interval_type>
  typename interval_traits<interval_type>::coordinate_type
  center(const interval_type& interval,
  typename enable_if< typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type * = 0
  ) { return (high(interval) + low(interval))/2; }


  struct y_i_low : gtl_yes {};

  // set the low coordinate to v
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_low, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, void>::type 
  low(interval_type& interval,
      typename interval_traits<interval_type>::coordinate_type v) { set(interval, LOW, v); }
  
  struct y_i_high : gtl_yes {};

  // set the high coordinate to v
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_high, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, void>::type 
  high(interval_type& interval,
      typename interval_traits<interval_type>::coordinate_type v) { set(interval, HIGH, v); }
  
  // get the magnitude of the interval
  template <typename interval_type>
  typename interval_difference_type<interval_type>::type 
  delta(const interval_type& interval,
  typename enable_if< typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type * = 0
  ) { 
    typedef typename coordinate_traits<typename interval_traits<interval_type>::coordinate_type>::coordinate_difference diffT;
    return (diffT)high(interval) - (diffT)low(interval); }

  struct y_i_flip : gtl_yes {};

  // flip this about coordinate
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_flip, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  flip(interval_type& interval,
       typename interval_traits<interval_type>::coordinate_type axis = 0) {
    typename interval_traits<interval_type>::coordinate_type newLow, newHigh;
    newLow  = 2 * axis - high(interval);
    newHigh = 2 * axis - low(interval);
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }

  struct y_i_scale_up : gtl_yes {};

  // scale interval by factor
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_scale_up, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  scale_up(interval_type& interval, 
           typename coordinate_traits<typename interval_traits<interval_type>::coordinate_type>::unsigned_area_type factor) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newHigh = high(interval) * (Unit)factor;
    low(interval, low(interval) * (Unit)factor);
    high(interval, (newHigh));
    return interval;
  }

  struct y_i_scale_down : gtl_yes {};

  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_scale_down, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  scale_down(interval_type& interval, 
             typename coordinate_traits<typename interval_traits<interval_type>::coordinate_type>::unsigned_area_type factor) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    typedef typename coordinate_traits<Unit>::coordinate_distance dt;
    Unit newHigh = scaling_policy<Unit>::round((dt)(high(interval)) / (dt)factor); 
    low(interval, scaling_policy<Unit>::round((dt)(low(interval)) / (dt)factor)); 
    high(interval, (newHigh));
    return interval;
  }

  struct y_i_scale : gtl_yes {};

  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_scale, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  scale(interval_type& interval, double factor) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newHigh = scaling_policy<Unit>::round((double)(high(interval)) * factor);
    low(interval, scaling_policy<Unit>::round((double)low(interval)* factor));
    high(interval, (newHigh));
    return interval;
  }
  
  // move interval by delta
  template <typename interval_type>
  interval_type&
  move(interval_type& interval,
       typename interval_difference_type<interval_type>::type displacement,
       typename enable_if<typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type * = 0
       ) {
    typedef typename interval_traits<interval_type>::coordinate_type ctype;
    typedef typename coordinate_traits<ctype>::coordinate_difference Unit;
    Unit len = delta(interval);
    low(interval, static_cast<ctype>(static_cast<Unit>(low(interval)) + displacement));
    high(interval, static_cast<ctype>(static_cast<Unit>(low(interval)) + len));
    return interval;
  }
  
  struct y_i_convolve : gtl_yes {};

  // convolve this with b
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_convolve, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  convolve(interval_type& interval,
           typename interval_traits<interval_type>::coordinate_type b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval) + b;
    Unit newHigh = high(interval) + b;
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }

  struct y_i_deconvolve : gtl_yes {};

  // deconvolve this with b
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_deconvolve, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type &
  deconvolve(interval_type& interval,
             typename interval_traits<interval_type>::coordinate_type b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval)  - b;
    Unit newHigh = high(interval) - b;
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }

  struct y_i_convolve2 : gtl_yes {};

  // convolve this with b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_convolve2,
                       typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type, 
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    interval_type>::type &
  convolve(interval_type& interval,
           const interval_type_2& b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval)  + low(b);
    Unit newHigh = high(interval) + high(b);
    low(interval, newLow);
                         high(interval, newHigh);
                         return interval;
  }
  
  struct y_i_deconvolve2 : gtl_yes {};

  // deconvolve this with b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3< y_i_deconvolve2,
                        typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
                        typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    interval_type>::type &
  deconvolve(interval_type& interval,
             const interval_type_2& b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval)  - low(b);
    Unit newHigh = high(interval) - high(b);
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }
  
  struct y_i_reconvolve : gtl_yes {};

  // reflected convolve this with b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_reconvolve,
                       typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    interval_type>::type &
  reflected_convolve(interval_type& interval,
                     const interval_type_2& b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval)  - high(b);
    Unit newHigh = high(interval) - low(b);
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }
  
  struct y_i_redeconvolve : gtl_yes {};

  // reflected deconvolve this with b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3< y_i_redeconvolve,
                        typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type, 
                        typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type, 
    interval_type>::type &
  reflected_deconvolve(interval_type& interval,
                       const interval_type_2& b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit newLow  = low(interval)  + high(b);
    Unit newHigh = high(interval) + low(b);
    low(interval, newLow);
    high(interval, newHigh);
    return interval;
  }
  
  struct y_i_e_dist1 : gtl_yes {};

  // distance from a coordinate to an interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_e_dist1, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       typename interval_difference_type<interval_type>::type>::type
  euclidean_distance(const interval_type& interval,
                     typename interval_traits<interval_type>::coordinate_type position) {
    typedef typename coordinate_traits<typename interval_traits<interval_type>::coordinate_type>::coordinate_difference Unit;
    Unit dist[3] = {0, (Unit)low(interval) - (Unit)position, (Unit)position - (Unit)high(interval)};
    return dist[ (dist[1] > 0) + ((dist[2] > 0) << 1) ];
  }

  struct y_i_e_dist2 : gtl_yes {};
  
  // distance between two intervals
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_e_dist2, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    typename interval_difference_type<interval_type>::type>::type
  euclidean_distance(const interval_type& interval,
                     const interval_type_2& b) {
    typedef typename coordinate_traits<typename interval_traits<interval_type>::coordinate_type>::coordinate_difference Unit;
    Unit dist[3] = {0, (Unit)low(interval) - (Unit)high(b), (Unit)low(b) - (Unit)high(interval)};
    return dist[ (dist[1] > 0) + ((dist[2] > 0) << 1) ];
  }
  
  struct y_i_e_intersects : gtl_yes {};

  // check if Interval b intersects `this` Interval
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_e_intersects, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    bool>::type 
    intersects(const interval_type& interval, const interval_type_2& b, 
               bool consider_touch = true) {
                         return consider_touch ? 
                           (low(interval) <= high(b)) & (high(interval) >= low(b)) :
                           (low(interval) < high(b)) & (high(interval) > low(b));
  }

  struct y_i_e_bintersect : gtl_yes {};

  // check if Interval b partially overlaps `this` Interval
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_e_bintersect, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    bool>::type 
  boundaries_intersect(const interval_type& interval, const interval_type_2& b, 
                       bool consider_touch = true) {
    return (contains(interval, low(b), consider_touch) || 
            contains(interval, high(b), consider_touch)) &&
      (contains(b, low(interval), consider_touch) || 
       contains(b, high(interval), consider_touch));
  }

  struct y_i_abuts1 : gtl_yes {};

  // check if they are end to end
  template <typename interval_type, typename interval_type_2>
  typename enable_if< typename gtl_and_3<y_i_abuts1, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
                                         typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
                       bool>::type 
  abuts(const interval_type& interval, const interval_type_2& b, direction_1d dir) {
    return dir.to_int() ? low(b) == high(interval) : low(interval) == high(b);
  }

  struct y_i_abuts2 : gtl_yes {};

  // check if they are end to end
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_abuts2, typename is_interval_concept<typename geometry_concept<interval_type>::type>::type,
                       typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    bool>::type 
  abuts(const interval_type& interval, const interval_type_2& b) {
    return abuts(interval, b, HIGH) || abuts(interval, b, LOW);
  } 

  struct y_i_intersect : gtl_yes {};

  // set 'this' interval to the intersection of 'this' and b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< typename gtl_and_3<y_i_intersect, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
                                         typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
                       bool>::type 
  intersect(interval_type& interval, const interval_type_2& b, bool consider_touch = true) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit lowVal = (std::max)(low(interval), low(b));
    Unit highVal = (std::min)(high(interval), high(b));
    bool valid = consider_touch ?
      lowVal <= highVal :
      lowVal < highVal;
    if(valid) {
      low(interval, lowVal);
      high(interval, highVal);
    }
    return valid;
  }

  struct y_i_g_intersect : gtl_yes {};

  // set 'this' interval to the generalized intersection of 'this' and b
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_g_intersect, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
                      typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    interval_type>::type &
  generalized_intersect(interval_type& interval, const interval_type_2& b) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit coords[4] = {low(interval), high(interval), low(b), high(b)};
    //consider implementing faster sorting of small fixed length range
    std::sort(coords, coords+4);
    low(interval, coords[1]);
    high(interval, coords[2]);
    return interval;
  }

  struct y_i_bloat : gtl_yes {};

  // bloat the Interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_bloat, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       interval_type>::type &
  bloat(interval_type& interval, typename interval_traits<interval_type>::coordinate_type bloating) {
    low(interval, low(interval)-bloating);
    high(interval, high(interval)+bloating);
    return interval;
  }
  
  struct y_i_bloat2 : gtl_yes {};

  // bloat the specified side of `this` Interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_bloat2, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       interval_type>::type &
  bloat(interval_type& interval, direction_1d dir, typename interval_traits<interval_type>::coordinate_type bloating) {
    set(interval, dir, get(interval, dir) + dir.get_sign() * bloating);
    return interval;
  }

  struct y_i_shrink : gtl_yes {};

  // shrink the Interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_shrink, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       interval_type>::type &
  shrink(interval_type& interval, typename interval_traits<interval_type>::coordinate_type shrinking) {
    return bloat(interval, -shrinking);
  }

  struct y_i_shrink2 : gtl_yes {};

  // shrink the specified side of `this` Interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_shrink2, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       interval_type>::type &
  shrink(interval_type& interval, direction_1d dir, typename interval_traits<interval_type>::coordinate_type shrinking) {
    return bloat(interval, dir, -shrinking);
  }

  // Enlarge `this` Interval to encompass the specified Interval
  template <typename interval_type, typename interval_type_2>
  bool
  encompass(interval_type& interval, const interval_type_2& b,
  typename enable_if<
    typename gtl_and< typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
            typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type>::type * = 0
  ) {
    bool retval = !contains(interval, b, true);
    low(interval, (std::min)(low(interval), low(b)));
    high(interval, (std::max)(high(interval), high(b)));
    return retval;
  }    

  struct y_i_encompass : gtl_yes {};

  // Enlarge `this` Interval to encompass the specified Interval
  template <typename interval_type>
  typename enable_if< typename gtl_and<y_i_encompass, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type,
                       bool>::type
  encompass(interval_type& interval, typename interval_traits<interval_type>::coordinate_type b) {
    bool retval = !contains(interval, b, true);
    low(interval, (std::min)(low(interval), b));
    high(interval, (std::max)(high(interval), b));
    return retval;
  }    

  struct y_i_get_half : gtl_yes {};

  // gets the half of the interval as an interval
  template <typename interval_type>
  typename enable_if<typename gtl_and<y_i_get_half, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type>::type, interval_type>::type 
  get_half(const interval_type& interval, direction_1d d1d) {
    typedef typename interval_traits<interval_type>::coordinate_type Unit;
    Unit c = (get(interval, LOW) + get(interval, HIGH)) / 2;
    return construct<interval_type>((d1d == LOW) ? get(interval, LOW) : c,
                                    (d1d == LOW) ? c : get(interval, HIGH));
  }

  struct y_i_join_with : gtl_yes {};

  // returns true if the 2 intervals exactly touch at one value, like in  l1 <= h1 == l2 <= h2
  // sets the argument to the joined interval
  template <typename interval_type, typename interval_type_2>
  typename enable_if< 
    typename gtl_and_3<y_i_join_with, typename is_mutable_interval_concept<typename geometry_concept<interval_type>::type>::type,
                      typename is_interval_concept<typename geometry_concept<interval_type_2>::type>::type>::type,
    bool>::type 
  join_with(interval_type& interval, const interval_type_2& b) {
    if(abuts(interval, b)) {
      encompass(interval, b);
      return true;
    }
    return false;
  }

  template <class T>
  template <class T2>
  interval_data<T>& interval_data<T>::operator=(const T2& rvalue) {
    assign(*this, rvalue);
    return *this;
  }

  template <typename T>
  struct geometry_concept<interval_data<T> > {
    typedef interval_concept type;
  };
}
}
#endif
