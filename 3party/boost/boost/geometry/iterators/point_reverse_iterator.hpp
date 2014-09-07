// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_POINT_REVERSE_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_POINT_REVERSE_ITERATOR_HPP

#include <iterator>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_convertible.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>

namespace boost { namespace geometry
{


// MK:: need to add doc here
template <typename Geometry>
class point_reverse_iterator
    : public std::reverse_iterator<point_iterator<Geometry> >
{
private:
    typedef std::reverse_iterator<point_iterator<Geometry> > base;

    base* base_ptr()
    {
        return this;
    }

    base const* base_ptr() const
    {
        return this;
    }

    template <typename OtherGeometry> friend class point_reverse_iterator;
    template <typename G>
    friend inline point_reverse_iterator<G> points_rbegin(G&);

    template <typename G>
    friend inline point_reverse_iterator<G> points_rend(G&);

    point_reverse_iterator(base const& base_it) : base(base_it) {}

public:
    point_reverse_iterator() {}

    template <typename OtherGeometry>
    point_reverse_iterator(point_reverse_iterator<OtherGeometry> const& other)
        : base(*other.base_ptr())
    {
        static const bool is_conv = boost::is_convertible
            <
                std::reverse_iterator<point_iterator<Geometry> >,
                std::reverse_iterator<point_iterator<OtherGeometry> >
            >::value;

        BOOST_MPL_ASSERT_MSG((is_conv),
                             NOT_CONVERTIBLE,
                             (point_reverse_iterator<OtherGeometry>));
    }
};


// MK:: need to add doc here
template <typename Geometry>
inline point_reverse_iterator<Geometry>
points_rbegin(Geometry& geometry)
{
    return std::reverse_iterator
        <
            point_iterator<Geometry>
        >(points_end(geometry));
}


// MK:: need to add doc here
template <typename Geometry>
inline point_reverse_iterator<Geometry>
points_rend(Geometry& geometry)
{
    return std::reverse_iterator
        <
            point_iterator<Geometry>
        >(points_begin(geometry));
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ITERATORS_POINT_REVERSE_ITERATOR_HPP
