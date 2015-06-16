// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EQUALS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EQUALS_HPP


#include <boost/geometry/algorithms/equals.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace equals
{


template
<
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct nsphere_nsphere
{
    template <typename S1, typename S2>
    static inline bool apply(S1 const& s1, S2 const& s2)
    {
        if ( !geometry::math::equals(get<Dimension>(s1), get<Dimension>(s2)) )
        {
            return false;
        }
        return nsphere_nsphere<Dimension + 1, DimensionCount>::apply(s1, s2);
    }
};

template <std::size_t DimensionCount>
struct nsphere_nsphere<DimensionCount, DimensionCount>
{
    template <typename S1, typename S2>
    static inline bool apply(S1 const& , S2 const& )
    {
        return true;
    }
};


}} // namespace detail::equals
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename NSphere1, typename NSphere2, std::size_t DimensionCount, bool Reverse>
struct equals<NSphere1, NSphere2, nsphere_tag, nsphere_tag, DimensionCount, Reverse>
{
    template <typename S1, typename S2>
    static inline bool apply(S1 const& s1, S2 const& s2)
    {
        return detail::equals::nsphere_nsphere<0, DimensionCount>::apply(s1, s2)
            && geometry::math::equals(get_radius<0>(s1), get_radius<0>(s2));
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EQUALS_HPP

