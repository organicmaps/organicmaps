// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SIDE_INFO_HPP
#define BOOST_GEOMETRY_STRATEGIES_SIDE_INFO_HPP


#include <utility>


namespace boost { namespace geometry
{


/*!
\brief Class side_info: small class wrapping for sides (-1,0,1)
*/
class side_info
{
public :
    inline side_info(int side_a1 = 0, int side_a2 = 0,
            int side_b1 = 0, int side_b2 = 0)
    {
        sides[0].first = side_a1;
        sides[0].second = side_a2;
        sides[1].first = side_b1;
        sides[1].second = side_b2;
    }

    template <int Which>
    inline void set(int first, int second)
    {
        sides[Which].first = first;
        sides[Which].second = second;
    }

    template <int Which, int Index>
    inline int get() const
    {
        return Index == 0 ? sides[Which].first : sides[Which].second;
    }


    // Returns true if both lying on the same side WRT the other
    // (so either 1,1 or -1-1)
    template <int Which>
    inline bool same() const
    {
        return sides[Which].first * sides[Which].second == 1;
    }

    inline bool collinear() const
    {
        return sides[0].first == 0
            && sides[0].second == 0
            && sides[1].first == 0
            && sides[1].second == 0;
    }

    // If one of the segments is collinear, the other must be as well.
    // So handle it as collinear.
    // (In floating point margins it can occur that one of them is 1!)
    inline bool as_collinear() const
    {
        return sides[0].first * sides[0].second == 0
            || sides[1].first * sides[1].second == 0;
    }

    inline void reverse()
    {
        std::swap(sides[0], sides[1]);
    }

private :
    std::pair<int, int> sides[2];

};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SIDE_INFO_HPP
