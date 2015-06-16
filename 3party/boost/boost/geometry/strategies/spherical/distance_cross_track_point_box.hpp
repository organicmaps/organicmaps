// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP


#include <boost/geometry/core/access.hpp>

#include <boost/geometry/strategies/distance.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/calculation_type.hpp>



namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

template
<
    typename CalculationType = void,
    typename Strategy = haversine<double, CalculationType>
>
class cross_track_point_box
{
public:
    template <typename Point, typename Box>
    struct return_type
        : promote_floating_point
          <
              typename select_calculation_type
                  <
                      Point,
                      typename point_type<Box>::type,
                      CalculationType
                  >::type
          >
    {};

    inline cross_track_point_box()
    {}

    explicit inline cross_track_point_box(typename Strategy::radius_type const& r)
        : m_pp_strategy(r)
    {}

    inline cross_track_point_box(Strategy const& s)
        : m_pp_strategy(s)
    {}
    
    template <typename Point, typename Box>
    inline typename return_type<Point, Box>::type
    apply(Point const& point, Box const& box) const
    {

#if !defined(BOOST_MSVC)
        BOOST_CONCEPT_ASSERT
            (
                (concept::PointDistanceStrategy
                    <
                        Strategy, Point,
                        typename point_type<Box>::type
                    >)
            );
#endif

        typedef typename return_type<Point, Box>::type return_type;
        typedef typename point_type<Box>::type box_point_t;
        
        // Create (counterclockwise) array of points, the fifth one closes it
        // If every point is on the LEFT side (=1) or ON the border (=0)
        // the distance should be equal to 0.

        // TODO: This strategy as well as other cross-track strategies
        // and therefore e.g. spherical within(Point, Box) may not work
        // properly for a Box degenerated to a Segment or Point

        boost::array<box_point_t, 5> bp;
        geometry::detail::assign_box_corners_oriented<true>(box, bp);
        bp[4] = bp[0];

        for (int i = 1; i < 5; i++)
        {
            box_point_t const& p1 = bp[i - 1];
            box_point_t const& p2 = bp[i];

            return_type const crs_AD = geometry::detail::course<return_type>(p1, point);
            return_type const crs_AB = geometry::detail::course<return_type>(p1, p2);
            return_type const d_crs1 = crs_AD - crs_AB;
            return_type const sin_d_crs1 = sin(d_crs1);

            // this constant sin() is here to be consistent with the side strategy
            return_type const sigXTD = asin(sin(0.001) * sin_d_crs1);

            // If the point is on the right side of the edge
            if ( sigXTD > 0 )
            {
                return_type const crs_BA = crs_AB - geometry::math::pi<return_type>();
                return_type const crs_BD = geometry::detail::course<return_type>(p2, point);
                return_type const d_crs2 = crs_BD - crs_BA;

                return_type const projection1 = cos( d_crs1 );
                return_type const projection2 = cos( d_crs2 );

                if(projection1 > 0.0 && projection2 > 0.0)
                {
                    return_type const d1 = m_pp_strategy.apply(p1, point);
                    return_type const
                        XTD = radius()
                            * geometry::math::abs(
                                asin( sin( d1 / radius() ) * sin_d_crs1 )
                              );

                    return return_type(XTD);
                }
                else
                {
                    // OPTIMIZATION
                    // Return d1 if projection1 <= 0 and d2 if projection2 <= 0
                    // if both == 0 then return d1 or d2
                    // both shouldn't be < 0

                    return_type const d1 = m_pp_strategy.apply(p1, point);
                    return_type const d2 = m_pp_strategy.apply(p2, point);

                    return return_type((std::min)( d1 , d2 ));
                }
            }
        }

        // Return 0 if the point isn't on the right side of any segment
        return return_type(0);
    }

    inline typename Strategy::radius_type radius() const
    { return m_pp_strategy.radius(); }

private :

    Strategy m_pp_strategy;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename CalculationType, typename Strategy>
struct tag<cross_track_point_box<CalculationType, Strategy> >
{
    typedef strategy_tag_distance_point_box type;
};


template <typename CalculationType, typename Strategy, typename P, typename Box>
struct return_type<cross_track_point_box<CalculationType, Strategy>, P, Box>
    : cross_track_point_box<CalculationType, Strategy>::template return_type<P, Box>
{};


template <typename CalculationType, typename Strategy>
struct comparable_type<cross_track_point_box<CalculationType, Strategy> >
{
    // There is no shortcut, so the strategy itself is its comparable type
    typedef cross_track_point_box<CalculationType, Strategy>  type;
};


template
<
    typename CalculationType,
    typename Strategy
>
struct get_comparable<cross_track_point_box<CalculationType, Strategy> >
{
    typedef typename comparable_type
        <
            cross_track_point_box<CalculationType, Strategy>
        >::type comparable_type;
public :
    static inline comparable_type apply(
        cross_track_point_box<CalculationType, Strategy> const& strategy)
    {
        return cross_track_point_box<CalculationType, Strategy>(strategy.radius());
    }
};


template
<
    typename CalculationType,
    typename Strategy,
    typename P, typename Box
>
struct result_from_distance
    <
        cross_track_point_box<CalculationType, Strategy>,
        P,
        Box
    >
{
private :
    typedef typename cross_track_point_box
        <
            CalculationType, Strategy
        >::template return_type<P, Box> return_type;
public :
    template <typename T>
    static inline return_type apply(
        cross_track_point_box<CalculationType, Strategy> const& ,
        T const& distance)
    {
        return distance;
    }
};


template <typename Point, typename Box, typename Strategy>
struct default_strategy
    <
        point_tag, box_tag, Point, Box,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef cross_track_point_box
        <
            void,
            typename boost::mpl::if_
                <
                    boost::is_void<Strategy>,
                    typename default_strategy
                        <
                            point_tag, point_tag,
                            Point, typename point_type<Box>::type,
                            spherical_equatorial_tag, spherical_equatorial_tag
                        >::type,
                    Strategy
                >::type
        > type;
};


template <typename Box, typename Point, typename Strategy>
struct default_strategy
    <
        box_tag, point_tag, Box, Point,
        spherical_equatorial_tag, spherical_equatorial_tag,
        Strategy
    >
{
    typedef typename default_strategy
        <
            point_tag, box_tag, Point, Box,
            spherical_equatorial_tag, spherical_equatorial_tag,
            Strategy
        >::type type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_CROSS_TRACK_POINT_BOX_HPP
