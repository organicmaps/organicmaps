// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_REMOVE_MARKED_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_REMOVE_MARKED_HPP

// PROBABLY OBSOLETE
// as mark_spikes is now replaced by remove_spikes

#include <boost/range.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace remove_marked
{


template <typename Range, typename MarkMap>
struct range_remove_marked
{
    typedef typename strategy::side::services::default_strategy
    <
        typename cs_tag<Range>::type
    >::type side_strategy_type;

    typedef typename coordinate_type<Range>::type coordinate_type;


    static inline void apply(Range const& range_in, ring_identifier id,
                    Range& range_out, MarkMap const& mark_map)
    {
        typename MarkMap::const_iterator mit = mark_map.find(id);
        if (mit == mark_map.end())
        {
            range_out = range_in;
            return;
        }
        typedef typename MarkMap::mapped_type bit_vector_type;

        if (boost::size(range_in) != boost::size(mit->second))
        {
            throw std::runtime_error("ERROR in size of mark_map");
            return;
        }

        range_out.clear();

        typename boost::range_iterator<bit_vector_type const>::type bit = boost::begin(mit->second);
        for (typename boost::range_iterator<Range const>::type it = boost::begin(range_in);
            it != boost::end(range_in); ++it, ++bit)
        {
            bool const& marked = *bit;
            if (! marked)
            {
                range_out.push_back(*it);
            }
        }
    }
};


template <typename Polygon, typename MarkMap>
struct polygon_remove_marked
{
    static inline void apply(Polygon const& polygon_in, ring_identifier id,
                Polygon& polygon_out, MarkMap const& mark_map)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef range_remove_marked<ring_type, MarkMap> per_range;
        id.ring_index = -1;
        per_range::apply(exterior_ring(polygon_in), id, exterior_ring(polygon_out), mark_map);


        typename interior_return_type<Polygon const>::type rings_in
                    = interior_rings(polygon_in);
        typename interior_return_type<Polygon>::type rings_out
                    = interior_rings(polygon_out);

        rings_out.resize(boost::size(interior_rings(polygon_in)));
        BOOST_AUTO_TPL(out, boost::begin(rings_out));

        for (BOOST_AUTO_TPL(it, boost::begin(rings_in));
            it != boost::end(rings_in);
            ++it, ++out)
        {
            id.ring_index++;
            per_range::apply(*it, id, *out, mark_map);
        }
    }
};


template <typename MultiGeometry, typename MarkMap, typename SinglePolicy>
struct multi_remove_marked
{
    static inline void apply(MultiGeometry const& multi_in, ring_identifier id,
                MultiGeometry& multi_out, MarkMap const& mark_map)
    {
        id.multi_index = 0;

        multi_out.resize(boost::size(multi_in));

        typename boost::range_iterator<MultiGeometry>::type out = boost::begin(multi_out);
        for (typename boost::range_iterator<MultiGeometry const>::type
                it = boost::begin(multi_in);
            it != boost::end(multi_in);
            ++it, ++out)
        {
            SinglePolicy::apply(*it, id, *out, mark_map);
            id.multi_index++;
        }
    }
};


}} // namespace detail::remove_marked
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename Geometry,
    typename MarkMap
>
struct remove_marked
{
    static inline void apply(Geometry const&, ring_identifier, Geometry&, MarkMap const&)
    {}
};


template <typename Ring, typename MarkMap>
struct remove_marked<ring_tag, Ring, MarkMap>
    : detail::remove_marked::range_remove_marked<Ring, MarkMap>
{};



template <typename Polygon, typename MarkMap>
struct remove_marked<polygon_tag, Polygon, MarkMap>
    : detail::remove_marked::polygon_remove_marked<Polygon, MarkMap>
{};


template <typename MultiPolygon, typename MarkMap>
struct remove_marked<multi_polygon_tag, MultiPolygon, MarkMap>
    : detail::remove_marked::multi_remove_marked
        <
            MultiPolygon,
            MarkMap,
            detail::remove_marked::polygon_remove_marked
            <
                typename boost::range_value<MultiPolygon>::type,
                MarkMap
            >
        >
{};



} // namespace dispatch
#endif


/*!
    \ingroup remove_marked
    \tparam Geometry geometry type
    \param geometry the geometry to make remove_marked
*/
template <typename Geometry, typename MarkMap>
inline void remove_marked(Geometry const& geometry_in, Geometry& geometry_out,
            MarkMap const& mark_map)
{
    concept::check<Geometry>();

    ring_identifier id;
    dispatch::remove_marked
        <
            typename tag<Geometry>::type,
            Geometry,
            MarkMap
        >::apply(geometry_in, id, geometry_out, mark_map);
}





}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_REMOVE_MARKED_HPP
