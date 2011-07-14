// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP

#include <vector>
#include <iterator>

#include <boost/concept_check.hpp>

#include <boost/geometry/strategies/concepts/distance_concept.hpp>


namespace boost { namespace geometry { namespace concept
{


/*!
    \brief Checks strategy for simplify
    \ingroup simplify
*/
template <typename Strategy>
struct SimplifyStrategy
{
#ifndef DOXYGEN_NO_CONCEPT_MEMBERS
private :

    // 1) must define distance_strategy_type,
    //    defining point-segment distance strategy (to be checked)
    typedef typename Strategy::distance_strategy_type ds_type;


    struct checker
    {
        template <typename ApplyMethod>
        static void apply(ApplyMethod const&)
        {
            namespace ft = boost::function_types;
            typedef typename ft::parameter_types
                <
                    ApplyMethod
                >::type parameter_types;

            typedef typename boost::mpl::if_
                <
                    ft::is_member_function_pointer<ApplyMethod>,
                    boost::mpl::int_<1>,
                    boost::mpl::int_<0>
                >::type base_index;

            // 1: inspect and define both arguments of apply
            typedef typename boost::remove_const
                <
                    typename boost::remove_reference
                    <
                        typename boost::mpl::at
                            <
                                parameter_types,
                                base_index
                            >::type
                    >::type
                >::type point_type;



            BOOST_CONCEPT_ASSERT
                (
                    (concept::PointSegmentDistanceStrategy<ds_type>)
                );

            Strategy *str;
            std::vector<point_type> const* v1;
            std::vector<point_type> * v2;

            // 2) must implement method apply with arguments
            //    - Range
            //    - OutputIterator
            //    - floating point value
            str->apply(*v1, std::back_inserter(*v2), 1.0);

            boost::ignore_unused_variable_warning(str);
        }
    };

public :
    BOOST_CONCEPT_USAGE(SimplifyStrategy)
    {
        checker::apply(&ds_type::apply);

    }
#endif
};



}}} // namespace boost::geometry::concept

#endif // BOOST_GEOMETRY_STRATEGIES_CONCEPTS_SIMPLIFY_CONCEPT_HPP
