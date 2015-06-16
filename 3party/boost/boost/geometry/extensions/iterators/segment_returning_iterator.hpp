// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SEGMENT_RETURNING_ITERATOR_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SEGMENT_RETURNING_ITERATOR_HPP

// TODO: This is very experimental version of input iterator
// reading collection of points as segments - proof of concept.
// --mloskot

// TODO: Move to boost::iterator_adaptor

#include <iterator>

#include <boost/iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/algorithms/equals.hpp>

// Helper geometry
#include <boost/geometry/geometries/segment.hpp>

namespace boost { namespace geometry
{

template <typename Base, typename Point>
struct segment_returning_iterator
{
    typedef Base base_type;
    typedef Point point_type;
    typedef typename model::referring_segment<Point> segment_type;

    typedef std::input_iterator_tag iterator_category;
    typedef typename std::iterator_traits<Base>::difference_type difference_type;
    typedef segment_type value_type;
    typedef segment_type* pointer;
    typedef segment_type& reference;

    explicit segment_returning_iterator(Base const& end)
        : m_segment(p1 , p2)
        , m_prev(end)
        , m_it(end)
        , m_end(end)
    {
    }

    segment_returning_iterator(Base const& it, Base const& end)
        : m_segment(p1 , p2)
        , m_prev(it)
        , m_it(it)
        , m_end(end)
    {
        if (m_it != m_end)
        {
            BOOST_GEOMETRY_ASSERT(m_prev != m_end);
            ++m_it;
        }
    }

    reference operator*()
    {
        BOOST_GEOMETRY_ASSERT(m_it != m_end && m_prev != m_end);

        p1 = *m_prev;
        p2 = *m_it;

        return m_segment;
    }

    pointer operator->()
    {
        return &(operator*());
    }

    segment_returning_iterator& operator++()
    {
        ++m_prev;
        ++m_it;
        return *this;
    }

    segment_returning_iterator operator++(int)
    {
        segment_returning_iterator it(*this);
        ++(*this);
        return it;
    }

    Base const& base() const { return m_it; }

private:

    point_type p1;
    point_type p2;
    segment_type m_segment;

    Base m_prev;
    Base m_it;
    Base m_end;
};

template <typename Base, typename Point>
bool operator==(segment_returning_iterator<Base, Point> const& lhs,
                segment_returning_iterator<Base, Point> const& rhs)
{
    return (lhs.base() == rhs.base());
}

template <typename Base, typename Point>
bool operator!=(segment_returning_iterator<Base, Point> const& lhs,
                segment_returning_iterator<Base, Point> const& rhs)
{
    return (lhs.base() != rhs.base());
}

template <typename C>
inline segment_returning_iterator
<
    typename C::iterator,
    typename C::value_type
>
make_segment_returning_iterator(C& c)
{
    typedef typename C::iterator base_iterator;
    typedef typename C::value_type point_type;
    return segment_returning_iterator<base_iterator, point_type>(c.begin(), c.end());
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SEGMENT_RETURNING_ITERATOR_HPP
