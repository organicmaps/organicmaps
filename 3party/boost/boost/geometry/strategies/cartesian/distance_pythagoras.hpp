// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP


#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/core/access.hpp>

#include <boost/geometry/strategies/distance.hpp>

#include <boost/geometry/util/calculation_type.hpp>




namespace boost { namespace geometry
{

namespace strategy { namespace distance
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Point1, typename Point2, size_t I, typename T>
struct compute_pythagoras
{
    static inline T apply(Point1 const& p1, Point2 const& p2)
    {
        T const c1 = boost::numeric_cast<T>(get<I-1>(p2));
        T const c2 = boost::numeric_cast<T>(get<I-1>(p1));
        T const d = c1 - c2;
        return d * d + compute_pythagoras<Point1, Point2, I-1, T>::apply(p1, p2);
    }
};

template <typename Point1, typename Point2, typename T>
struct compute_pythagoras<Point1, Point2, 0, T>
{
    static inline T apply(Point1 const&, Point2 const&)
    {
        return boost::numeric_cast<T>(0);
    }
};

}
#endif // DOXYGEN_NO_DETAIL


namespace comparable
{

/*!
\brief Strategy to calculate comparable distance between two points
\ingroup strategies
\tparam Point1 \tparam_first_point
\tparam Point2 \tparam_second_point
\tparam CalculationType \tparam_calculation
*/
template
<
    typename Point1,
    typename Point2 = Point1,
    typename CalculationType = void
>
class pythagoras
{
public :

    typedef typename util::calculation_type::geometric::binary
            <
                Point1,
                Point2,
                CalculationType
            >::type calculation_type;

    static inline calculation_type apply(Point1 const& p1, Point2 const& p2)
    {
        BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point1>) );
        BOOST_CONCEPT_ASSERT( (concept::ConstPoint<Point2>) );

        // Calculate distance using Pythagoras
        // (Leave comment above for Doxygen)

        assert_dimension_equal<Point1, Point2>();

        return detail::compute_pythagoras
            <
                Point1, Point2,
                dimension<Point1>::value,
                calculation_type
            >::apply(p1, p2);
    }
};

} // namespace comparable


/*!
\brief Strategy to calculate the distance between two points
\ingroup strategies
\tparam Point1 \tparam_first_point
\tparam Point2 \tparam_second_point
\tparam CalculationType \tparam_calculation

\qbk{
[heading Notes]
[note Can be used for points with two\, three or more dimensions]
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename Point1,
    typename Point2 = Point1,
    typename CalculationType = void
>
class pythagoras
{
    typedef comparable::pythagoras<Point1, Point2, CalculationType> comparable_type;
public :
    typedef typename util::calculation_type::geometric::binary
            <
                Point1,
                Point2,
                CalculationType,
                double,
                double // promote integer to double
            >::type calculation_type;

    /*!
    \brief applies the distance calculation using pythagoras
    \return the calculated distance (including taking the square root)
    \param p1 first point
    \param p2 second point
    */
    static inline calculation_type apply(Point1 const& p1, Point2 const& p2)
    {
        calculation_type const t = comparable_type::apply(p1, p2);
        return sqrt(t);
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename Point1, typename Point2, typename CalculationType>
struct tag<pythagoras<Point1, Point2, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct return_type<pythagoras<Point1, Point2, CalculationType> >
{
    typedef typename pythagoras<Point1, Point2, CalculationType>::calculation_type type;
};


template
<
    typename Point1,
    typename Point2,
    typename CalculationType,
    typename P1,
    typename P2
>
struct similar_type<pythagoras<Point1, Point2, CalculationType>, P1, P2>
{
    typedef pythagoras<P1, P2, CalculationType> type;
};


template
<
    typename Point1,
    typename Point2,
    typename CalculationType,
    typename P1,
    typename P2
>
struct get_similar<pythagoras<Point1, Point2, CalculationType>, P1, P2>
{
    static inline typename similar_type
        <
            pythagoras<Point1, Point2, CalculationType>, P1, P2
        >::type apply(pythagoras<Point1, Point2, CalculationType> const& )
    {
        return pythagoras<P1, P2, CalculationType>();
    }
};


template <typename Point1, typename Point2, typename CalculationType>
struct comparable_type<pythagoras<Point1, Point2, CalculationType> >
{
    typedef comparable::pythagoras<Point1, Point2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct get_comparable<pythagoras<Point1, Point2, CalculationType> >
{
    typedef comparable::pythagoras<Point1, Point2, CalculationType> comparable_type;
public :
    static inline comparable_type apply(pythagoras<Point1, Point2, CalculationType> const& )
    {
        return comparable_type();
    }
};


template <typename Point1, typename Point2, typename CalculationType>
struct result_from_distance<pythagoras<Point1, Point2, CalculationType> >
{
private :
    typedef typename return_type<pythagoras<Point1, Point2, CalculationType> >::type return_type;
public :
    template <typename T>
    static inline return_type apply(pythagoras<Point1, Point2, CalculationType> const& , T const& value)
    {
        return return_type(value);
    }
};


// Specializations for comparable::pythagoras
template <typename Point1, typename Point2, typename CalculationType>
struct tag<comparable::pythagoras<Point1, Point2, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct return_type<comparable::pythagoras<Point1, Point2, CalculationType> >
{
    typedef typename comparable::pythagoras<Point1, Point2, CalculationType>::calculation_type type;
};




template
<
    typename Point1,
    typename Point2,
    typename CalculationType,
    typename P1,
    typename P2
>
struct similar_type<comparable::pythagoras<Point1, Point2, CalculationType>, P1, P2>
{
    typedef comparable::pythagoras<P1, P2, CalculationType> type;
};


template
<
    typename Point1,
    typename Point2,
    typename CalculationType,
    typename P1,
    typename P2
>
struct get_similar<comparable::pythagoras<Point1, Point2, CalculationType>, P1, P2>
{
    static inline typename similar_type
        <
            comparable::pythagoras<Point1, Point2, CalculationType>, P1, P2
        >::type apply(comparable::pythagoras<Point1, Point2, CalculationType> const& )
    {
        return comparable::pythagoras<P1, P2, CalculationType>();
    }
};


template <typename Point1, typename Point2, typename CalculationType>
struct comparable_type<comparable::pythagoras<Point1, Point2, CalculationType> >
{
    typedef comparable::pythagoras<Point1, Point2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct get_comparable<comparable::pythagoras<Point1, Point2, CalculationType> >
{
    typedef comparable::pythagoras<Point1, Point2, CalculationType> comparable_type;
public :
    static inline comparable_type apply(comparable::pythagoras<Point1, Point2, CalculationType> const& )
    {
        return comparable_type();
    }
};


template <typename Point1, typename Point2, typename CalculationType>
struct result_from_distance<comparable::pythagoras<Point1, Point2, CalculationType> >
{
private :
    typedef typename return_type<comparable::pythagoras<Point1, Point2, CalculationType> >::type return_type;
public :
    template <typename T>
    static inline return_type apply(comparable::pythagoras<Point1, Point2, CalculationType> const& , T const& value)
    {
        return_type const v = value;
        return v * v;
    }
};


template <typename Point1, typename Point2>
struct default_strategy<point_tag, Point1, Point2, cartesian_tag, cartesian_tag, void>
{
    typedef pythagoras<Point1, Point2> type;
};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_PYTHAGORAS_HPP
