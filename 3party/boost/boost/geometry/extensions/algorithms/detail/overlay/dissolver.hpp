// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_DISSOLVER_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_DISSOLVER_HPP


#include <deque>
#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/union.hpp>
#include <boost/geometry/algorithms/reverse.hpp>


#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL

namespace detail { namespace inserter
{


template<typename Tag1, typename Tag2>
struct insert_geometry
{};

template<>
struct insert_geometry<ring_tag, polygon_tag>
{
    template<typename Ring, typename Collection>
    static inline void apply(Ring const& ring, Collection& collection)
    {
        collection.resize(collection.size() + 1);
        geometry::exterior_ring(collection.back()) = ring;
    }
};




template<>
struct insert_geometry<polygon_tag, polygon_tag>
{
    template<typename Geometry, typename Collection>
    static inline void apply(Geometry const& geometry, Collection& collection)
    {
        collection.push_back(geometry);
    }
};

template<typename Geometry, typename Collection>
inline void insert(Geometry const& geometry, Collection& collection)
{
    insert_geometry
        <
            typename geometry::tag<Geometry>::type,
            typename geometry::tag
                <
                    typename boost::range_value<Collection>::type
                >::type
        >::apply(geometry, collection);
}

}} // namespace detail::inserter



namespace detail { namespace dissolver
{

class plusmin_policy
{
    template
    <
        typename Geometry1,
        typename Geometry2,
        typename RescalePolicy,
        typename OutputCollection
    >
    static inline bool check_negative(Geometry1 a, Geometry2 b, // pass-by-value
                    RescalePolicy const& rescale_policy,
                    OutputCollection& output_collection)
    {
        // Precondition: a = positive, b = negative

        // 1: make b positive to get proper intersection
        geometry::reverse(b);
        {
            // 2: Check if there is overlap
            OutputCollection difference;
            geometry::intersection(a, b, difference);
            if(difference.size() <= 0)
            {
                return false;
            }
        }

        // There is overlap and we want to remove it, by subtracting it from b

        //negative = true;

        typedef typename geometry::point_type<Geometry2>::type point_type;

        typedef overlay::turn_info
            <
                point_type,
                typename segment_ratio_type<point_type, RescalePolicy>::type
            > turn_info;
        std::deque<turn_info> turns;

        // Get (and stop on) any intersection
        detail::disjoint::disjoint_interrupt_policy policy;
        geometry::get_turns
            <
                false, false,
                overlay::assign_null_policy
            >(a, b, rescale_policy, turns, policy);

        if (! policy.has_intersections)
        {
            // There is overlap but no intersections -> b is inside a.
            // So keep A and keep B, do not change anything
            return false;
        }

        // There are intersections.
        // 3: make a negative
        geometry::reverse(a); // now negative

        // This will calculate B minus A, result is then positive
        OutputCollection difference;
        geometry::intersection(a, b, difference);

        // Add original a to output (NOT necessary! TODO avoid this)
        {
            geometry::reverse(a); // positive again
            detail::inserter::insert(a, output_collection);
        }

        // Make negative output negative again
        typedef typename boost::range_iterator<OutputCollection>::type iterator_type;
        for(iterator_type it = boost::begin(difference);
            it != boost::end(difference);
            ++it)
        {
            geometry::reverse(*it);
            detail::inserter::insert(*it, output_collection);
        }
        return true;
    }


public :

    template
    <
        typename Geometry1,
        typename Geometry2,
        typename RescalePolicy,
        typename OutputCollection
    >
    static inline bool apply(Geometry1 const& a, Geometry2 const& b,
                    RescalePolicy const& rescale_policy,
                    OutputCollection& output_collection)
    {
        typedef typename geometry::coordinate_type<Geometry2>::type coordinate_type;
        coordinate_type area_a = geometry::area(a);
        coordinate_type area_b = geometry::area(b);

        // DEBUG
        /*
        int n = boost::size(output_collection);
        typedef typename geometry::point_type<Geometry2>::type point_type;
        std::cout << "Combine "
            << area_a << " with " << " " << area_b
            << " { " << geometry::wkt(geometry::return_centroid<point_type>(a))
            << geometry::wkt(geometry::return_centroid<point_type>(b)) << " }"
             << std::endl;
        */
        // END DEBUG

        coordinate_type zero = coordinate_type();
        if (area_a > zero && area_b > zero)
        {
            geometry::union_(a, b, output_collection);
            return true;
        }
        else if (area_a > zero && area_b < zero)
        {
            return check_negative(a, b, rescale_policy, output_collection);
        }
        else if (area_a < zero && area_b > zero)
        {
            return check_negative(b, a, rescale_policy, output_collection);
        }

        // both negative (?) TODO
        // DEBUG
        /*
        for (int i = n; i < boost::size(output_collection); i++)
        {
            typedef typename geometry::point_type<Geometry2>::type point_type;
            std::cout << "Result "
                << geometry::area(output_collection[i])
                << " " << geometry::wkt(geometry::return_centroid<point_type>(output_collection[i]))
                << std::endl;
        }
        */
        // END DEBUG
        return false;

    }

};


template <typename CombinePolicy>
struct dissolver_generic
{


    // Small structure to access elements by index;
    // this avoids copying or accessing elements by address (pointer)
    template <typename Box>
    struct dissolve_helper
    {
        int source; // 0,1
        int index; // index in the original array
        bool dissolved;
        Box box;
        double area;

        dissolve_helper()
        {}

        dissolve_helper(int i, Box b, double a, int s)
            : source(s)
            , index(i)
            , dissolved(false)
            , box(b)
            , area(a)
        {}
    };


    struct get_geometry
    {
        template <typename Range>
        inline static typename boost::range_value<Range>::type const& apply(
            Range const& range, int index)
        {
            return range[index];
        }
    };

    template
    <
        typename Vector,
        typename HelperVector
    >
    static inline void init_helper(Vector const& v, HelperVector& helper,
        int index = 0, int source = 0)
    {
        typedef typename boost::range_value<Vector>::type value_type;
        typedef typename geometry::point_type<value_type>::type point_type;
        typedef model::box<point_type> box_type;
        for(typename boost::range_iterator<Vector const>::type
            it = boost::begin(v);
            it != boost::end(v);
            ++it, ++index)
        {
            helper.push_back(dissolve_helper<box_type>(index,
                    geometry::return_envelope<box_type>(*it),
                    geometry::area(*it),
                    source));
        }
    }

    template
    <
        typename Element,
        typename Geometry1, typename Geometry2,
        typename RescalePolicy,
        typename OutputCollection
    >
    static inline bool call_policy(
            Element const& , Element const& ,
            Geometry1 const& geometry1, Geometry2 const& geometry2,
            RescalePolicy const& rescale_policy,
            OutputCollection& output_collection)
    {
        if (! geometry::disjoint(geometry1, geometry2))
        {
            /*std::cout << "Process " << element1.source << "/" << element1.index
                << " and " << element2.source << "/" << element2.index
                << "  (" << element2.dissolved << "," << element2.dissolved << ")"
                << std::endl;
            */
            return CombinePolicy::apply(geometry1, geometry2,
                            rescale_policy, output_collection);
        }
        return false;
    }


    template
    <
        int Dimension,
        typename HelperVector,
        typename IndexVector,
        typename InputRange,
        typename RescalePolicy,
        typename OutputCollection,
        typename Box
    >
    static inline bool divide_and_conquer(HelperVector& helper_vector
                , IndexVector& index_vector
                , InputRange const& input_range
                , RescalePolicy const& rescale_policy
                , OutputCollection& output_collection
                , Box const& total_box
                , bool& changed
                , int iteration = 0
                )
    {
        //std::cout << "divide_and_conquer " << iteration << std::endl;
        typedef typename geometry::coordinate_type<Box>::type coordinate_type;
        typedef typename boost::range_value<HelperVector>::type helper_type;
        typedef typename boost::range_iterator<IndexVector const>::type iterator_type;

        //if (boost::size(index_vector) >= 16 && iteration < 100)
        // Not yet using divide and conquer
        if (false)
        {
            // 1: separate box into 2 (either horizontally or vertically)
            Box lower_box = total_box, upper_box = total_box;
            coordinate_type two = 2.0;
            coordinate_type mid
                = (geometry::get<min_corner, Dimension>(total_box)
                    + geometry::get<max_corner, Dimension>(total_box)) / two;

            geometry::set<max_corner, Dimension>(lower_box, mid);
            geometry::set<min_corner, Dimension>(upper_box, mid);

            // 2: divide indices into two sublists
            IndexVector lower_list, upper_list;
            for(iterator_type it = boost::begin(index_vector);
                it != boost::end(index_vector);
                ++it)
            {
                helper_type const& element = helper_vector[*it];
                if (! geometry::disjoint(lower_box, element.box))
                {
                    lower_list.push_back(*it);
                }
                if (! geometry::disjoint(upper_box, element.box))
                {
                    upper_list.push_back(*it);
                }
            }

            //std::cout << lower_list.size() << ", " << upper_list.size()<< std::endl;

            // 3: recursively call function (possibly divide in other dimension)
            divide_and_conquer<1 - Dimension>(helper_vector,
                lower_list, input_range, rescale_policy, output_collection, lower_box, changed, iteration + 1);
            divide_and_conquer<1 - Dimension>(helper_vector,
                upper_list, input_range, rescale_policy, output_collection, upper_box, changed, iteration + 1);
            return changed;
        }

        // There are less then 16 elements, handle them quadraticly

        int n = boost::size(output_collection);

        for(iterator_type it1 = boost::begin(index_vector);
            it1 != boost::end(index_vector);
            ++it1)
        {
            helper_type& element1 = helper_vector[*it1];

            bool unioned = false;
            for(iterator_type it2 = boost::begin(index_vector);
                ! unioned && it2 != it1;
                ++it2)
            {
                helper_type& element2 = helper_vector[*it2];

                // If they are NOT disjoint, union them
                if (! element1.dissolved
                    && ! element2.dissolved
                    && ! geometry::disjoint(element1.box, element2.box))
                {
                    // Runtime type check here...
                    if ((element1.source == 0 && element2.source == 0
                        && call_policy
                            (
                                element1, element2,
                                get_geometry::apply(input_range, element1.index),
                                get_geometry::apply(input_range, element2.index),
                                rescale_policy,
                                output_collection
                            )
                        )
                        || (element1.source == 0 && element2.source == 1
                        && call_policy
                            (
                                element1, element2,
                                get_geometry::apply(input_range, element1.index),
                                get_geometry::apply(output_collection, element2.index),
                                rescale_policy,
                                output_collection
                            )
                        )
                        || (element1.source == 1 && element2.source == 0
                        && call_policy
                            (
                                element1, element2,
                                get_geometry::apply(output_collection, element1.index),
                                get_geometry::apply(input_range, element2.index),
                                rescale_policy,
                                output_collection
                            )
                        )
                        || (element1.source == 1 && element2.source == 1
                        && call_policy
                            (
                                element1, element2,
                                get_geometry::apply(output_collection, element1.index),
                                get_geometry::apply(output_collection, element2.index),
                                rescale_policy,
                                output_collection
                            )
                        )
                        )
                    {
                        changed = true;
                        element1.dissolved = true;
                        element2.dissolved = true;

                        unioned = true;
/*std::cout << "Assign " << element1.source << "/" << element1.index
<< " and " << element2.source << "/" << element2.index
<< "  (" << element2.dissolved << "," << element2.dissolved << ")"
<< std::endl;
*/
                    }
                }
            }
        }

        // Append new records in output collection to helper class
        init_helper(std::make_pair(boost::begin(output_collection) + n,
            boost::end(output_collection)), helper_vector, n, 1);

        return changed;
    }

    template <typename T>
    static inline bool helper_dissolved(T const& t)
    {
      return t.dissolved;
    }



    template
    <
        typename InputRange,
        typename RescalePolicy,
        typename OutputCollection
    >
    static inline void apply(InputRange const& input_range
                , RescalePolicy const& rescale_policy
                , OutputCollection& output_collection
                )
    {
        typedef typename boost::range_value<OutputCollection>::type output_type;

        typedef typename geometry::point_type<output_type>::type point_type;
        typedef model::box<point_type> box_type;
        typedef dissolve_helper<box_type> dissolve_helper_type;
        typedef std::vector<dissolve_helper_type> helper_vector_type;

        // Vector with indices to both input_range (source 0) and output_collection (source 1)
        helper_vector_type helper_vector;

        // Vector with indices to helper-vector, for divide and conquer
        std::vector<int> index_vector;


        init_helper(input_range, helper_vector);

        // Fill intrusive list with copies, and determine bounding box
        box_type total_box;
        geometry::assign_inverse(total_box);
        int index = 0;
        for(typename boost::range_iterator<helper_vector_type const>::type
            it = boost::begin(helper_vector);
            it != boost::end(helper_vector);
            ++it, ++index)
        {
            index_vector.push_back(index);
            geometry::expand(total_box, it->box);
        }

        std::vector<output_type> unioned_collection;

        int size = 0, previous_size = 0;
        int n = 0;

        bool changed = false;
        while(divide_and_conquer<1>
            (helper_vector, index_vector, input_range, rescale_policy, unioned_collection, total_box, changed) && n < 5)
        {
            // Remove everything which is already dissolved.
            helper_vector.erase
                (
                    std::remove_if
                        (
                            helper_vector.begin(),
                            helper_vector.end(),
                            helper_dissolved<dissolve_helper_type>
                        ),
                    helper_vector.end()
                );

            previous_size = size;
            size = helper_vector.size();
            n = previous_size == size ? n + 1 : 0;

            // Re-initialize the list
            index_vector.clear();
            index = 0;
            for(typename boost::range_iterator<helper_vector_type const>::type
                it = boost::begin(helper_vector);
                it != boost::end(helper_vector);
                ++it, ++index)
            {
                index_vector.push_back(index);
            }

            changed = false;

            //std::cout << " " << size;
        }

        // Add input+output to real output
        typedef typename boost::range_iterator<helper_vector_type>::type iterator_type;
        for(iterator_type it = boost::begin(helper_vector);
            it != boost::end(helper_vector);
            ++it)
        {
            if (! it->dissolved)
            {
                switch(it->source)
                {
                    case 0 :
                        detail::inserter::insert(
                            get_geometry::apply(input_range, it->index),
                            output_collection);
                        break;
                    case 1 :
                        detail::inserter::insert(
                            get_geometry::apply(unioned_collection, it->index),
                            output_collection);
                        break;
                }
            }
        }
    }
};


}} // namespace detail::dissolver
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag1,
    typename GeometryTag2,
    typename Policy
>
struct dissolver
{};


template<typename Policy>
struct dissolver<ring_tag, polygon_tag, Policy>
    : detail::dissolver::dissolver_generic<Policy>
{};

template<typename Policy>
struct dissolver<polygon_tag, polygon_tag, Policy>
    : detail::dissolver::dissolver_generic<Policy>
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template
<
    typename InputRange,
    typename OutputCollection
>
inline void dissolver(InputRange const& input_range,
        OutputCollection& output_collection)
{
    typedef typename boost::range_value<InputRange>::type geometry_in;
    typedef typename boost::range_value<OutputCollection>::type geometry_out;
    concept::check<geometry_in const>();
    concept::check<geometry_out>();

    dispatch::dissolver
    <
        typename tag<geometry_in>::type,
        typename tag<geometry_out>::type,
        detail::dissolver::plusmin_policy
    >::apply(input_range, output_collection);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_DISSOLVER_HPP
