// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014, 2015.
// Modifications copyright (c) 2014-2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef TTMATH_STUB
#define TTMATH_STUB

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/coordinate_cast.hpp>


#include <ttmath.h>
namespace ttmath
{
    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> sqrt(Big<Exponent, Mantissa> const& v)
    {
        return Sqrt(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> abs(Big<Exponent, Mantissa> const& v)
    {
        return Abs(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> ceil(Big<Exponent, Mantissa> const& v)
    {
        return Ceil(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> floor(Big<Exponent, Mantissa> const& v)
    {
        return Floor(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> asin(Big<Exponent, Mantissa> const& v)
    {
        return ASin(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> sin(Big<Exponent, Mantissa> const& v)
    {
        return Sin(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> cos(Big<Exponent, Mantissa> const& v)
    {
        return Cos(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> tan(Big<Exponent, Mantissa> const& v)
    {
        return Tan(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> atan(Big<Exponent, Mantissa> const& v)
    {
        return ATan(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> acos(Big<Exponent, Mantissa> const& v)
    {
        return ACos(v);
    }

    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> atan2(Big<Exponent, Mantissa> const& y, Big<Exponent, Mantissa> const& x)
    {
        // return ATan2(y, 2); does not (yet) exist in ttmath...

        // See http://en.wikipedia.org/wiki/Atan2

        Big<Exponent, Mantissa> const zero(0);
        Big<Exponent, Mantissa> const two(2);

        if (y == zero)
        {
            // return x >= 0 ? 0 : pi and pi=2*arccos(0)
            return x >= zero ? zero : two * ACos(zero);
        }

        return two * ATan((sqrt(x * x + y * y) - x) / y);
    }

    // needed in order to work with boost::geometry::math::mod
    template <uint Exponent, uint Mantissa>
    inline Big<Exponent, Mantissa> mod(Big<Exponent, Mantissa> const& x,
                                       Big<Exponent, Mantissa> const& y)
    {
        return Mod(x, y);
    }
}

// Specific structure implementing constructor
// (WHICH IS NECESSARY FOR Boost.Geometry because it enables using T() !! )
struct ttmath_big : ttmath::Big<1,4>
{
    ttmath_big(double v = 0)
        : ttmath::Big<1,4>(v)
    {}
    ttmath_big(ttmath::Big<1,4> const& v)
        : ttmath::Big<1,4>(v)
    {}

    // unary operator+() is implemented for completeness
    inline ttmath_big const& operator+() const
    {
        return *this;
    }

    // needed in order to work with boost::geometry::math::abs
    inline ttmath_big operator-() const
    {
        return ttmath::Big<1,4>::operator-();
    }

    /*
    inline operator double() const
    {
        return atof(this->ToString().c_str());
    }

    inline operator int() const
    {
        return atol(ttmath::Round(*this).ToString().c_str());
    }
    */
};


// arithmetic operators for ttmath_big objects, defined as free functions
inline ttmath_big operator+(ttmath_big const& x, ttmath_big const& y)
{
    return static_cast<ttmath::Big<1,4> const&>(x).operator+(y);
}

inline ttmath_big operator-(ttmath_big const& x, ttmath_big const& y)
{
    return static_cast<ttmath::Big<1,4> const&>(x).operator-(y);
}

inline ttmath_big operator*(ttmath_big const& x, ttmath_big const& y)
{
    return static_cast<ttmath::Big<1,4> const&>(x).operator*(y);
}

inline ttmath_big operator/(ttmath_big const& x, ttmath_big const& y)
{
    return static_cast<ttmath::Big<1,4> const&>(x).operator/(y);
}


namespace boost{ namespace geometry { namespace math
{

namespace detail
{
    // Workaround for boost::math::constants::pi:
    // 1) lexical cast -> stack overflow and
    // 2) because it is implemented as a function, generic implementation not possible

    // Partial specialization for ttmath
    template <ttmath::uint Exponent, ttmath::uint Mantissa>
    struct define_half_pi<ttmath::Big<Exponent, Mantissa> >
    {
        static inline ttmath::Big<Exponent, Mantissa> apply()
        {
            static ttmath::Big<Exponent, Mantissa> const half_pi(
                "1.57079632679489661923132169163975144209858469968755291048747229615390820314310449931401741267105853399107404325664115332354692230477529111586267970406424055872514205135096926055277982231147447746519098");
            return half_pi;
        }
    };

    // Partial specialization for ttmath
    template <ttmath::uint Exponent, ttmath::uint Mantissa>
    struct define_pi<ttmath::Big<Exponent, Mantissa> >
    {
        static inline ttmath::Big<Exponent, Mantissa> apply()
        {
            static ttmath::Big<Exponent, Mantissa> const the_pi(
                "3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848111745028410270193852110555964462294895493038196");
            return the_pi;
        }
    };

    // Partial specialization for ttmath
    template <ttmath::uint Exponent, ttmath::uint Mantissa>
    struct define_two_pi<ttmath::Big<Exponent, Mantissa> >
    {
        static inline ttmath::Big<Exponent, Mantissa> apply()
        {
            static ttmath::Big<Exponent, Mantissa> const two_pi(
                "6.28318530717958647692528676655900576839433879875021164194988918461563281257241799725606965068423413596429617302656461329418768921910116446345071881625696223490056820540387704221111928924589790986076392");
            return two_pi;
        }
    };

    template <>
    struct define_half_pi<ttmath_big>
            : public define_half_pi<ttmath::Big<1,4> >
    {};

    template <>
    struct define_pi<ttmath_big>
            : public define_pi<ttmath::Big<1,4> >
    {};

    template <>
    struct define_two_pi<ttmath_big>
            : public define_two_pi<ttmath::Big<1,4> >
    {};

    template <ttmath::uint Exponent, ttmath::uint Mantissa>
    struct equals_with_epsilon<ttmath::Big<Exponent, Mantissa>, false>
    {
        static inline bool apply(ttmath::Big<Exponent, Mantissa> const& a, ttmath::Big<Exponent, Mantissa> const& b)
        {
            // See implementation in util/math.hpp
            // But here borrow the tolerance for double, to avoid exact comparison
            ttmath::Big<Exponent, Mantissa> const epsilon = std::numeric_limits<double>::epsilon();
            return ttmath::Abs(a - b) <= epsilon * ttmath::Abs(a);
        }
    };

    template <>
    struct equals_with_epsilon<ttmath_big, false>
            : public equals_with_epsilon<ttmath::Big<1, 4>, false>
    {};

} // detail

} // ttmath


namespace detail
{

template <ttmath::uint Exponent, ttmath::uint Mantissa>
struct coordinate_cast<ttmath::Big<Exponent, Mantissa> >
{
    static inline ttmath::Big<Exponent, Mantissa> apply(std::string const& source)
    {
        return ttmath::Big<Exponent, Mantissa> (source);
    }
};


template <>
struct coordinate_cast<ttmath_big>
{
    static inline ttmath_big apply(std::string const& source)
    {
        return ttmath_big(source);
    }
};

} // namespace detail


}} // boost::geometry




// Support for boost::numeric_cast to int and to double (necessary for SVG-mapper)
namespace boost { namespace numeric
{

template
<
    ttmath::uint Exponent, ttmath::uint Mantissa,
    typename Traits,
    typename OverflowHandler,
    typename Float2IntRounder,
    typename RawConverter,
    typename UserRangeChecker
>
struct converter<int, ttmath::Big<Exponent, Mantissa>, Traits, OverflowHandler, Float2IntRounder, RawConverter, UserRangeChecker>
{
    static inline int convert(ttmath::Big<Exponent, Mantissa> arg)
    {
        int v;
        arg.ToInt(v);
        return v;
    }
};

template
<
    ttmath::uint Exponent, ttmath::uint Mantissa,
    typename Traits,
    typename OverflowHandler,
    typename Float2IntRounder,
    typename RawConverter,
    typename UserRangeChecker
>
struct converter<double, ttmath::Big<Exponent, Mantissa>, Traits, OverflowHandler, Float2IntRounder, RawConverter, UserRangeChecker>
{
    static inline double convert(ttmath::Big<Exponent, Mantissa> arg)
    {
        double v;
        arg.ToDouble(v);
        return v;
    }
};


template
<
    typename Traits,
    typename OverflowHandler,
    typename Float2IntRounder,
    typename RawConverter,
    typename UserRangeChecker
>
struct converter<int, ttmath_big, Traits, OverflowHandler, Float2IntRounder, RawConverter, UserRangeChecker>
{
    static inline int convert(ttmath_big arg)
    {
        int v;
        arg.ToInt(v);
        return v;
    }
};

template
<
    typename Traits,
    typename OverflowHandler,
    typename Float2IntRounder,
    typename RawConverter,
    typename UserRangeChecker
>
struct converter<double, ttmath_big, Traits, OverflowHandler, Float2IntRounder, RawConverter, UserRangeChecker>
{
    static inline double convert(ttmath_big arg)
    {
        double v;
        arg.ToDouble(v);
        return v;
    }
};


}}


#endif
