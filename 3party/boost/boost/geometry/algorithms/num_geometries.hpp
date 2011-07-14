// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP

#include <cstddef>

#include <boost/mpl/assert.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Tag, typename Geometry>
struct num_geometries
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry>)
        );
};


template <typename Geometry>
struct num_geometries<single_tag, Geometry>
{
    static inline std::size_t apply(Geometry const&)
    {
        return 1;
    }
};



} // namespace dispatch
#endif


/*!
\brief \brief_calc{number of geometries}
\ingroup num_geometries
\details \details_calc{num_geometries, number of geometries}.
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{number of geometries}

\qbk{[include reference/algorithms/num_geometries.qbk]}
*/
template <typename Geometry>
inline std::size_t num_geometries(Geometry const& geometry)
{
    concept::check<Geometry const>();

    return dispatch::num_geometries
        <
            typename tag_cast
                <
                    typename tag<Geometry>::type,
                    single_tag,
                    multi_tag
                >::type,
            Geometry
        >::apply(geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_NUM_GEOMETRIES_HPP
