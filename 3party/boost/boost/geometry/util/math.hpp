// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_MATH_HPP
#define BOOST_GEOMETRY_UTIL_MATH_HPP

#include <cmath>
#include <limits>

#include <boost/type_traits/is_fundamental.hpp>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace math
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Type, bool IsFloatingPoint>
struct equals
{
    static inline bool apply(Type const& a, Type const& b)
    {
        return a == b;
    }
};

template <typename Type>
struct equals<Type, true>
{
    static inline Type get_max(Type const& a, Type const& b, Type const& c)
    {
        return (std::max)((std::max)(a, b), c);
    }

    static inline bool apply(Type const& a, Type const& b)
    {
        if (a == b)
        {
            return true;
        }

        // See http://www.parashift.com/c++-faq-lite/newbie.html#faq-29.17,
        // FUTURE: replace by some boost tool or boost::test::close_at_tolerance
        return std::abs(a - b) <= std::numeric_limits<Type>::epsilon() * get_max(std::abs(a), std::abs(b), 1.0);
    }
};

template <typename Type, bool IsFloatingPoint>
struct smaller
{
    static inline bool apply(Type const& a, Type const& b)
    {
        return a < b;
    }
};

template <typename Type>
struct smaller<Type, true>
{
    static inline bool apply(Type const& a, Type const& b)
    {
        if (equals<Type, true>::apply(a, b))
        {
            return false;
        }
        return a < b;
    }
};


template <typename Type, bool IsFloatingPoint>
struct equals_with_epsilon : public equals<Type, IsFloatingPoint> {};

template
<
    typename T,
    bool IsFundemantal = boost::is_fundamental<T>::value /* false */
>
struct square_root
{
    typedef T return_type;

    static inline T apply(T const& value)
    {
        // for non-fundamental number types assume that sqrt is
        // defined either:
        // 1) at T's scope, or
        // 2) at global scope, or
        // 3) in namespace std
        using ::sqrt;
        using std::sqrt;

        return sqrt(value);
    }
};

template <>
struct square_root<float, true>
{
    typedef float return_type;

    static inline float apply(float const& value)
    {
        // for float use std::sqrt
        return std::sqrt(value);
    }
};

template <>
struct square_root<long double, true>
{
    typedef long double return_type;

    static inline long double apply(long double const& value)
    {
        // for long double use std::sqrt
        return std::sqrt(value);
    }
};

template <typename T>
struct square_root<T, true>
{
    typedef double return_type;

    static inline double apply(T const& value)
    {
        // for all other fundamental number types use also std::sqrt
        //
        // Note: in C++98 the only other possibility is double;
        //       in C++11 there are also overloads for integral types;
        //       this specialization works for those as well.
        return std::sqrt(boost::numeric_cast<double>(value));
    }
};


/*!
\brief Short construct to enable partial specialization for PI, currently not possible in Math.
*/
template <typename T>
struct define_pi
{
    static inline T apply()
    {
        // Default calls Boost.Math
        return boost::math::constants::pi<T>();
    }
};

template <typename T>
struct relaxed_epsilon
{
    static inline T apply(const T& factor)
    {
        return factor * std::numeric_limits<T>::epsilon();
    }
};

// ItoF ItoI FtoF
template <typename Result, typename Source,
          bool ResultIsInteger = std::numeric_limits<Result>::is_integer,
          bool SourceIsInteger = std::numeric_limits<Source>::is_integer>
struct round
{
    static inline Result apply(Source const& v)
    {
        return boost::numeric_cast<Result>(v);
    }
};

// FtoI
template <typename Result, typename Source>
struct round<Result, Source, true, false>
{
    static inline Result apply(Source const& v)
    {
        return v < 0
             ? boost::numeric_cast<Result>(ceil(v - 0.5f))
             : boost::numeric_cast<Result>(floor(v + 0.5f));        
    }
};

} // namespace detail
#endif


template <typename T>
inline T pi() { return detail::define_pi<T>::apply(); }

template <typename T>
inline T relaxed_epsilon(T const& factor)
{
    return detail::relaxed_epsilon<T>::apply(factor);
}


// Maybe replace this by boost equals or boost ublas numeric equals or so

/*!
    \brief returns true if both arguments are equal.
    \ingroup utility
    \param a first argument
    \param b second argument
    \return true if a == b
    \note If both a and b are of an integral type, comparison is done by ==.
    If one of the types is floating point, comparison is done by abs and
    comparing with epsilon. If one of the types is non-fundamental, it might
    be a high-precision number and comparison is done using the == operator
    of that class.
*/

template <typename T1, typename T2>
inline bool equals(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::equals
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool equals_with_epsilon(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::equals_with_epsilon
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool smaller(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::smaller
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool larger(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::smaller
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(b, a);
}



double const d2r = geometry::math::pi<double>() / 180.0;
double const r2d = 1.0 / d2r;

/*!
    \brief Calculates the haversine of an angle
    \ingroup utility
    \note See http://en.wikipedia.org/wiki/Haversine_formula
    haversin(alpha) = sin2(alpha/2)
*/
template <typename T>
inline T hav(T const& theta)
{
    T const half = T(0.5);
    T const sn = sin(half * theta);
    return sn * sn;
}

/*!
\brief Short utility to return the square
\ingroup utility
\param value Value to calculate the square from
\return The squared value
*/
template <typename T>
inline T sqr(T const& value)
{
    return value * value;
}

/*!
\brief Short utility to return the square root
\ingroup utility
\param value Value to calculate the square root from
\return The square root value
*/
template <typename T>
inline typename detail::square_root<T>::return_type
sqrt(T const& value)
{
    return detail::square_root
        <
            T, boost::is_fundamental<T>::value
        >::apply(value);
}

/*!
\brief Short utility to workaround gcc/clang problem that abs is converting to integer
       and that older versions of MSVC does not support abs of long long...
\ingroup utility
*/
template<typename T>
inline T abs(T const& value)
{
    T const zero = T();
    return value < zero ? -value : value;
}

/*!
\brief Short utility to calculate the sign of a number: -1 (negative), 0 (zero), 1 (positive)
\ingroup utility
*/
template <typename T>
static inline int sign(T const& value)
{
    T const zero = T();
    return value > zero ? 1 : value < zero ? -1 : 0;
}

/*!
\brief Short utility to calculate the rounded value of a number.
\ingroup utility
\note If the source T is NOT an integral type and Result is an integral type
      the value is rounded towards the closest integral value. Otherwise it's
      just casted.
*/
template <typename Result, typename T>
inline Result round(T const& v)
{
    // NOTE: boost::round() could be used instead but it throws in some situations
    return detail::round<Result, T>::apply(v);
}

} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_MATH_HPP
