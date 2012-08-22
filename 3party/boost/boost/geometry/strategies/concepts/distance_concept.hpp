// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CONCEPTS_DISTANCE_CONCEPT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONCEPTS_DISTANCE_CONCEPT_HPP

#include <vector>
#include <iterator>

#include <boost/concept_check.hpp>

#include <boost/geometry/util/parameter_type_of.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry { namespace concept
{


/*!
    \brief Checks strategy for point-segment-distance
    \ingroup distance
*/
template <typename Strategy>
struct PointDistanceStrategy
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
private :

    struct checker
    {
        template <typename ApplyMethod>
        static void apply(ApplyMethod const&)
        {
            // 1: inspect and define both arguments of apply
            typedef typename parameter_type_of
                <
                    ApplyMethod, 0
                >::type ptype1;

            typedef typename parameter_type_of
                <
                    ApplyMethod, 1
                >::type ptype2;

            // 2) check if apply-arguments fulfill point concept
            BOOST_CONCEPT_ASSERT
                (
                    (concept::ConstPoint<ptype1>)
                );

            BOOST_CONCEPT_ASSERT
                (
                    (concept::ConstPoint<ptype2>)
                );


            // 3) must define meta-function return_type
            typedef typename strategy::distance::services::return_type<Strategy>::type rtype;

            // 4) must define meta-function "similar_type"
            typedef typename strategy::distance::services::similar_type
                <
                    Strategy, ptype2, ptype1
                >::type stype;

            // 5) must define meta-function "comparable_type"
            typedef typename strategy::distance::services::comparable_type
                <
                    Strategy
                >::type ctype;

            // 6) must define meta-function "tag"
            typedef typename strategy::distance::services::tag
                <
                    Strategy
                >::type tag;

            // 7) must implement apply with arguments
            Strategy* str = 0;
            ptype1 *p1 = 0;
            ptype2 *p2 = 0;
            rtype r = str->apply(*p1, *p2);

            // 8) must define (meta)struct "get_similar" with apply
            stype s = strategy::distance::services::get_similar
                <
                    Strategy,
                    ptype2, ptype1
                >::apply(*str);

            // 9) must define (meta)struct "get_comparable" with apply
            ctype c = strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(*str);

            // 10) must define (meta)struct "result_from_distance" with apply
            r = strategy::distance::services::result_from_distance
                <
                    Strategy
                >::apply(*str, 1.0);

            boost::ignore_unused_variable_warning(str);
            boost::ignore_unused_variable_warning(s);
            boost::ignore_unused_variable_warning(c);
            boost::ignore_unused_variable_warning(r);
        }
    };



public :
    BOOST_CONCEPT_USAGE(PointDistanceStrategy)
    {
        checker::apply(&Strategy::apply);
    }
#endif
};


/*!
    \brief Checks strategy for point-segment-distance
    \ingroup strategy_concepts
*/
template <typename Strategy>
struct PointSegmentDistanceStrategy
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
private :

    struct checker
    {
        template <typename ApplyMethod>
        static void apply(ApplyMethod const&)
        {
            typedef typename parameter_type_of
                <
                    ApplyMethod, 0
                >::type ptype;

            typedef typename parameter_type_of
                <
                    ApplyMethod, 1
                >::type sptype;

            // 2) check if apply-arguments fulfill point concept
            BOOST_CONCEPT_ASSERT
                (
                    (concept::ConstPoint<ptype>)
                );

            BOOST_CONCEPT_ASSERT
                (
                    (concept::ConstPoint<sptype>)
                );


            // 3) must define meta-function return_type
            typedef typename strategy::distance::services::return_type<Strategy>::type rtype;

            // 4) must define underlying point-distance-strategy
            typedef typename strategy::distance::services::strategy_point_point<Strategy>::type stype;
            BOOST_CONCEPT_ASSERT
                (
                    (concept::PointDistanceStrategy<stype>)
                );


            Strategy *str = 0;
            ptype *p = 0;
            sptype *sp1 = 0;
            sptype *sp2 = 0;

            rtype r = str->apply(*p, *sp1, *sp2);

            boost::ignore_unused_variable_warning(str);
            boost::ignore_unused_variable_warning(r);
        }
    };

public :
    BOOST_CONCEPT_USAGE(PointSegmentDistanceStrategy)
    {
        checker::apply(&Strategy::apply);
    }
#endif
};


}}} // namespace boost::geometry::concept


#endif // BOOST_GEOMETRY_STRATEGIES_CONCEPTS_DISTANCE_CONCEPT_HPP
