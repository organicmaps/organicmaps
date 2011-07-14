// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP


#include <string>

#include <boost/concept_check.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>


namespace boost { namespace geometry
{

namespace policies { namespace relate
{


template <typename S1, typename S2, typename ReturnType, typename CalculationType = void>
struct segments_intersection_points
{
    typedef ReturnType return_type;
    typedef S1 segment_type1;
    typedef S2 segment_type2;
    typedef typename select_calculation_type
        <
            S1, S2, CalculationType
        >::type coordinate_type;

    // Get the same type, but at least a double (also used for divisions
    typedef typename select_most_precise
        <
            coordinate_type, double
        >::type promoted_type;

    template <int Dimension>
    static inline return_type rico(
                    coordinate_type const& dm1, coordinate_type const& dn1,
                    coordinate_type const& dm2, coordinate_type const& dn2,
                    S1 const& s1, S2 const& s2)
    {
        promoted_type const a1 = dn1 / dm1;
        promoted_type const a2 = dn2 / dm2;
        promoted_type const da = a1 - a2;

        if (math::equals(da, 0))
        {
            return rico<1 - Dimension>(dn1, dm1, dn2, dm2, s1, s2);
        }

        promoted_type const b1 = get<0, Dimension>(s1) - a1 * get<0, 1 - Dimension>(s1);
        promoted_type const b2 = get<0, Dimension>(s2) - a2 * get<0, 1 - Dimension>(s2);

        promoted_type const v = (b2 - b1) / da;

        return_type result;
        result.count = 1;
        set<1 - Dimension>(result.intersections[0],
            boost::numeric_cast<coordinate_type>(v));
        set<Dimension>(result.intersections[0],
            boost::numeric_cast<coordinate_type>(a1 * v + b1));
        return result;
    }

    static inline return_type cross(S1 const& s1, S2 const& s2)
    {
        // Take one of first segment, and one of second segment
        return_type result;
        result.count = 1;
        set<0>(result.intersections[0], get<0, 0>(s1));
        set<1>(result.intersections[0], get<0, 1>(s2));
        return result;
    }


    static inline return_type segments_intersect(side_info const& sides,
                    coordinate_type const& dx1, coordinate_type const& dy1,
                    coordinate_type const& dx2, coordinate_type const& dy2,
                    S1 const& s1, S2 const& s2)
    {
        bool vertical1 = math::equals(dx1, 0);
        bool horizontal2 = math::equals(dy2, 0);
        if (vertical1 && horizontal2)
        {
            return cross(s1, s2);
        }

        bool vertical2 = math::equals(dx2, 0);
        bool horizontal1 = math::equals(dy1, 0);
        if (horizontal1 && vertical2)
        {
            return cross(s2, s1);
        }
        if (vertical1 || vertical2)
        {
            return rico<0>(dy1, dx1, dy2, dx2, s1, s2);
        }
        else
        {
            // Not crossing, take the most reasonable choice.
            // We want to divide by the largest one.
            //if (

            return rico<1>(dx1, dy1, dx2, dy2, s1, s2);
        }
    }

    static inline return_type collinear_touch(coordinate_type const& x,
                coordinate_type const& y, bool, char)
    {
        return_type result;
        result.count = 1;
        set<0>(result.intersections[0], x);
        set<1>(result.intersections[0], y);
        return result;
    }

    template <typename S>
    static inline return_type collinear_inside(S const& s)
    {
        return_type result;
        result.count = 2;
        set<0>(result.intersections[0], get<0, 0>(s));
        set<1>(result.intersections[0], get<0, 1>(s));
        set<0>(result.intersections[1], get<1, 0>(s));
        set<1>(result.intersections[1], get<1, 1>(s));
        return result;
    }

    template <typename S>
    static inline return_type collinear_interior_boundary_intersect(S const& s, bool, bool)
    {
        return collinear_inside(s);
    }

    static inline return_type collinear_a_in_b(S1 const& s, bool)
    {
        return collinear_inside(s);
    }
    static inline return_type collinear_b_in_a(S2 const& s, bool)
    {
        return collinear_inside(s);
    }

    static inline return_type collinear_overlaps(
                coordinate_type const& x1, coordinate_type const& y1,
                coordinate_type const& x2, coordinate_type const& y2, bool)
    {
        return_type result;
        result.count = 2;
        set<0>(result.intersections[0], x1);
        set<1>(result.intersections[0], y1);
        set<0>(result.intersections[1], x2);
        set<1>(result.intersections[1], y2);
        return result;
    }

    static inline return_type segment_equal(S1 const& s, bool)
    {
        return_type result;
        result.count = 2;
        set<0>(result.intersections[0], get<0, 0>(s));
        set<1>(result.intersections[0], get<0, 1>(s));
        set<0>(result.intersections[1], get<1, 0>(s));
        set<1>(result.intersections[1], get<1, 1>(s));
        return result;
    }

    static inline return_type disjoint()
    {
        return return_type();
    }
    static inline return_type error(std::string const& msg)
    {
        return return_type();
    }

    static inline return_type collinear_disjoint()
    {
        return return_type();
    }
    static inline return_type parallel()
    {
        return return_type();
    }
    static inline return_type degenerate(S1 const& s, bool)
    {
        return_type result;
        result.count = 1;
        set<0>(result.intersections[0], get<0, 0>(s));
        set<1>(result.intersections[0], get<0, 1>(s));
        return result;
    }
};


}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP
