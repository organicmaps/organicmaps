// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP

#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/range.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


/// Calculate envelope of an 2D or 3D segment
struct envelope_expand_one
{
    template<typename Geometry, typename Box>
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
struct envelope_range
{
    /// Calculate envelope of range using a strategy
    template <typename Range, typename Box>
    static inline void apply(Range const& range, Box& mbr)
    {
        assign_inverse(mbr);
        envelope_range_additional(range, mbr);
    }
};


struct envelope_multi_linestring
{
    template<typename MultiLinestring, typename Box>
    static inline void apply(MultiLinestring const& mp, Box& mbr)
    {
        assign_inverse(mbr);
        for (typename boost::range_iterator<MultiLinestring const>::type
                it = mp.begin();
            it != mp.end();
            ++it)
        {
            envelope_range_additional(*it, mbr);
        }
    }
};


// version for multi_polygon: outer ring's of all polygons
struct envelope_multi_polygon
{
    template<typename MultiPolygon, typename Box>
    static inline void apply(MultiPolygon const& mp, Box& mbr)
    {
        assign_inverse(mbr);
        for (typename boost::range_const_iterator<MultiPolygon>::type
                it = mp.begin();
            it != mp.end();
            ++it)
        {
            envelope_range_additional(exterior_ring(*it), mbr);
        }
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct envelope: not_implemented<Tag>
{};


template <typename Point>
struct envelope<Point, point_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Box>
struct envelope<Box, box_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Segment>
struct envelope<Segment, segment_tag>
    : detail::envelope::envelope_expand_one
{};


template <typename Linestring>
struct envelope<Linestring, linestring_tag>
    : detail::envelope::envelope_range
{};


template <typename Ring>
struct envelope<Ring, ring_tag>
    : detail::envelope::envelope_range
{};


template <typename Polygon>
struct envelope<Polygon, polygon_tag>
    : detail::envelope::envelope_range
{
    template <typename Box>
    static inline void apply(Polygon const& poly, Box& mbr)
    {
        // For polygon, inspecting outer ring is sufficient
        detail::envelope::envelope_range::apply(exterior_ring(poly), mbr);
    }

};


template <typename Multi>
struct envelope<Multi, multi_point_tag>
    : detail::envelope::envelope_range
{};


template <typename Multi>
struct envelope<Multi, multi_linestring_tag>
    : detail::envelope::envelope_multi_linestring
{};


template <typename Multi>
struct envelope<Multi, multi_polygon_tag>
    : detail::envelope::envelope_multi_polygon
{};


} // namespace dispatch
#endif


namespace resolve_variant {

template <typename Geometry>
struct envelope
{
    template <typename Box>
    static inline void apply(Geometry const& geometry, Box& box)
    {
        concept::check<Geometry const>();
        concept::check<Box>();

        dispatch::envelope<Geometry>::apply(geometry, box);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct envelope<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename Box>
    struct visitor: boost::static_visitor<void>
    {
        Box& m_box;

        visitor(Box& box): m_box(box) {}

        template <typename Geometry>
        void operator()(Geometry const& geometry) const
        {
            envelope<Geometry>::apply(geometry, m_box);
        }
    };

    template <typename Box>
    static inline void
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          Box& box)
    {
        boost::apply_visitor(visitor<Box>(box), geometry);
    }
};

} // namespace resolve_variant


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{envelope,\det_envelope}.
\tparam Geometry \tparam_geometry
\tparam Box \tparam_box
\param geometry \param_geometry
\param mbr \param_box \param_set{envelope}

\qbk{[include reference/algorithms/envelope.qbk]}
\qbk{
[heading Example]
[envelope] [envelope_output]
}
*/
template<typename Geometry, typename Box>
inline void envelope(Geometry const& geometry, Box& mbr)
{
    resolve_variant::envelope<Geometry>::apply(geometry, mbr);
}


/*!
\brief \brief_calc{envelope}
\ingroup envelope
\details \details_calc{return_envelope,\det_envelope}. \details_return{envelope}
\tparam Box \tparam_box
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{envelope}

\qbk{[include reference/algorithms/envelope.qbk]}
\qbk{
[heading Example]
[return_envelope] [return_envelope_output]
}
*/
template<typename Box, typename Geometry>
inline Box return_envelope(Geometry const& geometry)
{
    Box mbr;
    resolve_variant::envelope<Geometry>::apply(geometry, mbr);
    return mbr;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_ENVELOPE_HPP
