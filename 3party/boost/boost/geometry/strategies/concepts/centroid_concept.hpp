// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CONCEPTS_CENTROID_CONCEPT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONCEPTS_CENTROID_CONCEPT_HPP



#include <boost/concept_check.hpp>


namespace boost { namespace geometry { namespace concept
{


/*!
    \brief Checks strategy for centroid
    \ingroup centroid
*/
template <typename Strategy>
class CentroidStrategy
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS

    // 1) must define state_type,
    typedef typename Strategy::state_type state_type;

    // 2) must define point_type,
    typedef typename Strategy::point_type point_type;

    // 3) must define point_type, of polygon (segments)
    typedef typename Strategy::segment_point_type spoint_type;

    struct check_methods
    {
        static void apply()
        {
            Strategy *str;
            state_type *st;

            // 4) must implement a static method apply,
            // getting two segment-points
            spoint_type const* sp;
            str->apply(*sp, *sp, *st);

            // 5) must implement a static method result
            //  getting the centroid
            point_type *c;
            bool r = str->result(*st, *c);

            boost::ignore_unused_variable_warning(str);
            boost::ignore_unused_variable_warning(r);
        }
    };

public :
    BOOST_CONCEPT_USAGE(CentroidStrategy)
    {
        check_methods::apply();
    }
#endif
};


}}} // namespace boost::geometry::concept


#endif // BOOST_GEOMETRY_STRATEGIES_CONCEPTS_CENTROID_CONCEPT_HPP
