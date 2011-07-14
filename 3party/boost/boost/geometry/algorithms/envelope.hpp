// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP

#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


/// Calculate envelope of an 2D or 3D segment
template<typename Geometry, typename Box>
struct envelope_expand_one
{
    static inline void apply(Geometry const& geometry, Box& mbr)
    {
        assign_inverse(mbr);
        geometry::expand(mbr, geometry);
    }
};


/// Iterate through range (also used in multi*)
template<typename Range, typename Box>
inline void envelope_range_additional(Range const& range, Box& mbr)
{
    typedef typename boost::range_iterator<Range const>::type iterator_type;

    for (iterator_type it = boost::begin(range);
        it != boost::end(range);
        ++it)
    {
        geometry::expand(mbr, *it);
    }
}



/// Generic range dispatching struct
template <typename Range, typename Box>
struct envelope_range
{
    /// Calculate envelope of range using a strategy
    static inline void apply(Range const& range, Box& mbr)
    {
        assign_inverse(mbr);
        envelope_range_additional(range, mbr);
    }
};

}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// Note, the strategy is for future use (less/greater -> compare spherical
// using other methods), defaults are OK for now.
// However, they are already in the template methods

template
<
    typename Tag1, typename Tag2,
    typename Geometry, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry>)
        );
};


template
<
    typename Point, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        point_tag, box_tag,
        Point, Box,
        StrategyLess, StrategyGreater
    >
    : detail::envelope::envelope_expand_one<Point, Box>
{};


template
<
    typename BoxIn, typename BoxOut,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        box_tag, box_tag,
        BoxIn, BoxOut,
        StrategyLess, StrategyGreater
    >
    : detail::envelope::envelope_expand_one<BoxIn, BoxOut>
{};


template
<
    typename Segment, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        segment_tag, box_tag,
        Segment, Box,
        StrategyLess, StrategyGreater
    >
    : detail::envelope::envelope_expand_one<Segment, Box>
{};


template
<
    typename Linestring, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        linestring_tag, box_tag,
        Linestring, Box,
        StrategyLess, StrategyGreater
    >
    : detail::envelope::envelope_range<Linestring, Box>
{};


template
<
    typename Ring, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        ring_tag, box_tag,
        Ring, Box,
        StrategyLess, StrategyGreater
    >
    : detail::envelope::envelope_range<Ring, Box>
{};


template
<
    typename Polygon, typename Box,
    typename StrategyLess, typename StrategyGreater
>
struct envelope
    <
        polygon_tag, box_tag,
        Polygon, Box,
        StrategyLess, StrategyGreater
    >
{
    static inline void apply(Polygon const& poly, Box& mbr)
    {
        // For polygon, inspecting outer ring is sufficient

        detail::envelope::envelope_range
            <
                typename ring_type<Polygon>::type,
                Box
            >::apply(exterior_ring(poly), mbr);
    }

};


} // namespace dispatch
#endif


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{envelope,\det_envelope}.
\tparam Geometry \tparam_geometry
\tparam Box \tparam_box
\param geometry \param_geometry
\param mbr \param_box \param_set{envelope}

\par Example:
Example showing envelope calculation, using point_ll latlong points
\dontinclude doxygen_1.cpp
\skip example_envelope_polygon
\line {
\until }


\qbk{
[heading Example]
[envelope] [envelope_output]
}
*/
template<typename Geometry, typename Box>
inline void envelope(Geometry const& geometry, Box& mbr)
{
    concept::check<Geometry const>();
    concept::check<Box>();

    dispatch::envelope
        <
            typename tag<Geometry>::type, typename tag<Box>::type,
            Geometry, Box,
            void, void
        >::apply(geometry, mbr);
}


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{return_envelope,\det_envelope}. \details_return{envelope}
\tparam Box \tparam_box
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{envelope}

\qbk{
[heading Example]
[return_envelope] [return_envelope_output]
}
*/
template<typename Box, typename Geometry>
inline Box return_envelope(Geometry const& geometry)
{
    concept::check<Geometry const>();
    concept::check<Box>();

    Box mbr;
    dispatch::envelope
        <
            typename tag<Geometry>::type, typename tag<Box>::type,
            Geometry, Box,
            void, void
        >::apply(geometry, mbr);
    return mbr;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
