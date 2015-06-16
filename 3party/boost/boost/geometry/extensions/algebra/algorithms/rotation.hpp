// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_ROTATION_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_ROTATION_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/extensions/algebra/algorithms/detail.hpp>

#include <boost/geometry/extensions/algebra/geometries/concepts/rotation_quaternion_concept.hpp>

// TODO - for multiplication of coordinates
// if coordinate_type is_integral - use double as the result type

namespace boost { namespace geometry {

namespace detail { namespace rotation {

template <typename V1, typename V2, typename Rotation, typename Tag1, typename Tag2, std::size_t Dimension>
struct matrix
{
    BOOST_MPL_ASSERT_MSG(false, NOT_IMPLEMENTED_FOR_THIS_DIMENSION, (Rotation));
};

template <typename V1, typename V2, typename Rotation>
struct matrix<V1, V2, Rotation, vector_tag, vector_tag, 3>
{
    static const bool cs_check =
        ::boost::is_same<typename traits::coordinate_system<V1>::type, cs::cartesian>::value &&
        ::boost::is_same<typename traits::coordinate_system<V2>::type, cs::cartesian>::value;

    BOOST_MPL_ASSERT_MSG(cs_check, NOT_IMPLEMENTED_FOR_THOSE_SYSTEMS, (V1, V2));

    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<V1>::type,
        typename traits::coordinate_type<V2>::type
    >::type cv_type;

    typedef typename geometry::select_most_precise<
        cv_type,
        typename traits::coordinate_type<Rotation>::type
    >::type cr_type;

    typedef model::vector<cv_type, 3> vector_type;

    inline static void apply(V1 const& v1, V2 const& v2, Rotation & r)
    {
        namespace da = detail::algebra;

        // TODO - should store coordinates in more precise variables before the normalization?

        // angle
        cv_type d = da::dot<0, 0, 3>(v1, v2);
        cv_type l =
            math::sqrt(da::dot<0, 0, 3>(v1, v1) * da::dot<0, 0, 3>(v2, v2));
        cv_type c = d / l;

        // rotation angle == 0
        // not needed really, because in this case function still returns zero-rotation
        if ( 1 - std::numeric_limits<cv_type>::epsilon() <= c )
        {
            set<0, 0>(r, 1); set<0, 1>(r, 0); set<0, 2>(r, 0);
            set<1, 0>(r, 0); set<1, 1>(r, 1); set<1, 2>(r, 0);
            set<2, 0>(r, 0); set<2, 1>(r, 0); set<2, 2>(r, 1);
            return;
        }

        vector_type axis;

        // rotation angle = 180
        if ( c <= std::numeric_limits<cv_type>::epsilon() - 1 )
        {
            // find arbitrary rotation axis perpendicular to v1
            da::cross<0, 0, 0>(vector_type(1, 0, 0), v1, axis);
            if ( da::dot<0, 0, 3>(axis, axis) < std::numeric_limits<cr_type>::epsilon() )
                da::cross<0, 0, 0>(vector_type(0, 1, 0), v1, axis);
        }
        else
        {
            // rotation axis
            da::cross<0, 0, 0>(v1, v2, axis);
        }

        // sin
        cv_type s = math::sqrt(1 - c * c);
        cv_type t = 1 - c;
        // normalize axis
        da::normalize<0, 3>(axis);

        cv_type txx = t*get<0>(axis)*get<0>(axis);
        cv_type tyy = t*get<1>(axis)*get<1>(axis);
        cv_type tzz = t*get<2>(axis)*get<2>(axis);
        cv_type txy = t*get<0>(axis)*get<1>(axis);
        cv_type sx = s*get<0>(axis);
        cv_type txz = t*get<0>(axis)*get<2>(axis);
        cv_type sy = s*get<1>(axis);
        cv_type tyz = t*get<1>(axis)*get<2>(axis);
        cv_type sz = s*get<2>(axis);

        set<0, 0>(r, txx+c); set<0, 1>(r, txy-sz); set<0, 2>(r, txz+sy);
        set<1, 0>(r, txy+sz); set<1, 1>(r, tyy+c); set<1, 2>(r, tyz-sx);
        set<2, 0>(r, txz-sy); set<2, 1>(r, tyz+sx); set<2, 2>(r, tzz+c);
    }
};

template <typename V1, typename V2, typename Rotation>
struct matrix<V1, V2, Rotation, vector_tag, vector_tag, 2>
{
    static const bool cs_check =
        ::boost::is_same<typename traits::coordinate_system<V1>::type, cs::cartesian>::value &&
        ::boost::is_same<typename traits::coordinate_system<V2>::type, cs::cartesian>::value;

    BOOST_MPL_ASSERT_MSG(cs_check, NOT_IMPLEMENTED_FOR_THOSE_SYSTEMS, (V1, V2));

    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<V1>::type,
        typename traits::coordinate_type<V2>::type
    >::type cv_type;

    inline static void apply(V1 const& v1, V2 const& v2, Rotation & r)
    {
        namespace da = detail::algebra;

        // TODO - should store coordinates in more precise variables before the normalization?

        // angle
        cv_type d = da::dot<0, 0, 2>(v1, v2);
        cv_type l =
            math::sqrt(da::dot<0, 0, 2>(v1, v1) * da::dot<0, 0, 2>(v2, v2));
        cv_type c = d / l;

        // TODO return also if l == 0;

        // rotation angle == 0
        // not needed really, because in this case function still returns zero-rotation
        if ( 1 - std::numeric_limits<cv_type>::epsilon() <= c )
        {
            set<0, 0>(r, 1); set<0, 1>(r, 0);
            set<1, 0>(r, 0); set<1, 1>(r, 1);
        }
        // rotation angle = 180
        else if ( c <= std::numeric_limits<cv_type>::epsilon() - 1 )
        {
            set<0, 0>(r, -1); set<0, 1>(r, 0);
            set<1, 0>(r, 0); set<1, 1>(r, -1);
        }
        else
        {
            // sin
            cv_type s = (get<0>(v1) * get<1>(v2) - get<1>(v1) * get<0>(v2)) / l;

            set<0, 0>(r, c); set<0, 1>(r, -s);
            set<1, 0>(r, s); set<1, 1>(r, c);
        }
    }
};

}} // namespace detail::rotation

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch {

template <typename V1, typename V2, typename Rotation,
          typename Tag1 = typename tag<V1>::type,
          typename Tag2 = typename tag<V2>::type,
          typename RTag = typename tag<Rotation>::type
>
struct rotation
{
    BOOST_MPL_ASSERT_MSG(false, NOT_IMPLEMENTED_FOR_THOSE_TAGS, (Tag1, Tag2, Rotation));
};

template <typename V1, typename V2, typename Rotation>
struct rotation<V1, V2, Rotation, vector_tag, vector_tag, rotation_quaternion_tag>
{
    static const bool cs_check =
        ::boost::is_same<typename traits::coordinate_system<V1>::type, cs::cartesian>::value &&
        ::boost::is_same<typename traits::coordinate_system<V2>::type, cs::cartesian>::value;

    BOOST_MPL_ASSERT_MSG(cs_check, NOT_IMPLEMENTED_FOR_THOSE_SYSTEMS, (V1, V2));

    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<V1>::type,
        typename traits::coordinate_type<V2>::type
    >::type cv_type;

    typedef typename geometry::select_most_precise<
        cv_type,
        typename traits::coordinate_type<Rotation>::type
    >::type cr_type;

    typedef model::vector<cv_type, 3> vector_type;

    inline static void apply(V1 const& v1, V2 const& v2, Rotation & r)
    {
        namespace da = detail::algebra;

        // TODO - should store coordinates in more precise variables before the normalization?

        cv_type d = da::dot<0, 0, 3>(v1, v2); // l1 * l2 * cos
        cv_type l = math::sqrt(da::dot<0, 0, 3>(v1, v1) * da::dot<0, 0, 3>(v2, v2)); // l1 * l2
        cv_type w = l + d; // l1 * l2 * ( 1 + cos )

        // rotation angle == 0
        // not needed really, because in this case function still returns zero-rotation
        if ( 2*l-std::numeric_limits<cv_type>::epsilon() <= w )
        {
            set<0>(r, 1); set<0>(r, 0); set<0>(r, 0); set<0>(r, 0);
        }
        // rotation angle == pi
        else if ( w <= std::numeric_limits<cv_type>::epsilon() )
        {
            set<0>(r, 0);
            // find arbitrary rotation axis perpendicular to v1
            da::cross<0, 0, 1>(vector_type(1, 0, 0), v1, r);
            if ( da::dot<1, 1, 3>(r, r) < std::numeric_limits<cr_type>::epsilon() )
                da::cross<0, 0, 1>(vector_type(0, 1, 0), v1, r);

            // normalize axis
            da::normalize<1, 3>(r);
        }
        else
        {
            set<0>(r, w); // l1 * l2 * ( 1 + cos )
            // rotation axis
            da::cross<0, 0, 1>(v1, v2, r); // l1 * l2 * sin * UNITA

            // normalize quaternion
            da::normalize<0, 4>(r);
        }
    }
};

template <typename V1, typename V2, typename Rotation>
struct rotation<V1, V2, Rotation, vector_tag, vector_tag, rotation_matrix_tag>
    : detail::rotation::matrix<V1, V2, Rotation, vector_tag, vector_tag, traits::dimension<Rotation>::value>
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

template <typename V1, typename V2, typename Rotation>
inline void rotation(V1 const& v1, V2 const& v2, Rotation & r)
{
    concept::check_concepts_and_equal_dimensions<V1 const, V2 const>();
    // TODO - replace the following by check_equal_dimensions
    concept::check_concepts_and_equal_dimensions<V1 const, Rotation>();

    dispatch::rotation<V1, V2, Rotation>::apply(v1, v2, r);
}

template <typename Rotation, typename V1, typename V2>
inline Rotation return_rotation(V1 const& v1, V2 const& v2)
{
    Rotation r;
    translation(v1, v2, r);
    return r;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_ROTATION_HPP
