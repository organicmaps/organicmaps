// Boost.Geometry Index
//
// R-tree query iterators
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUERY_ITERATORS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUERY_ITERATORS_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename Value, typename Allocators>
struct end_query_iterator
{
    typedef std::input_iterator_tag iterator_category;
    typedef Value value_type;
    typedef typename Allocators::const_reference reference;
    typedef typename Allocators::difference_type difference_type;
    typedef typename Allocators::const_pointer pointer;

    reference operator*() const
    {
        BOOST_ASSERT_MSG(false, "iterator not dereferencable");
        pointer p(0);
        return *p;
    }

    const value_type * operator->() const
    {
        BOOST_ASSERT_MSG(false, "iterator not dereferencable");
        const value_type * p = 0;
        return p;
    }

    end_query_iterator & operator++()
    {
        BOOST_ASSERT_MSG(false, "iterator not incrementable");
        return *this;
    }

    end_query_iterator operator++(int)
    {
        BOOST_ASSERT_MSG(false, "iterator not incrementable");
        return *this;
    }

    friend bool operator==(end_query_iterator l, end_query_iterator r)
    {
        return true;
    }

    friend bool operator!=(end_query_iterator l, end_query_iterator r)
    {
        return false;
    }
};

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators, typename Predicates>
class spatial_query_iterator
{
    typedef visitors::spatial_query_incremental<Value, Options, Translator, Box, Allocators, Predicates> visitor_type;
    typedef typename visitor_type::node_pointer node_pointer;

public:
    typedef std::input_iterator_tag iterator_category;
    typedef Value value_type;
    typedef typename Allocators::const_reference reference;
    typedef typename Allocators::difference_type difference_type;
    typedef typename Allocators::const_pointer pointer;

    inline spatial_query_iterator(Translator const& t, Predicates const& p)
        : m_visitor(t, p)
    {}

    inline spatial_query_iterator(node_pointer root, Translator const& t, Predicates const& p)
        : m_visitor(t, p)
    {
        detail::rtree::apply_visitor(m_visitor, *root);
        m_visitor.increment();
    }

    reference operator*() const
    {
        return m_visitor.dereference();
    }

    const value_type * operator->() const
    {
        return boost::addressof(m_visitor.dereference());
    }

    spatial_query_iterator & operator++()
    {
        m_visitor.increment();
        return *this;
    }

    spatial_query_iterator operator++(int)
    {
        spatial_query_iterator temp = *this;
        this->operator++();
        return temp;
    }

    friend bool operator==(spatial_query_iterator const& l, spatial_query_iterator const& r)
    {
        return l.m_visitor == r.m_visitor;
    }

    friend bool operator==(spatial_query_iterator const& l, end_query_iterator<Value, Allocators>)
    {
        return l.m_visitor.is_end();
    }

    friend bool operator==(end_query_iterator<Value, Allocators>, spatial_query_iterator const& r)
    {
        return r.m_visitor.is_end();
    }

    friend bool operator!=(spatial_query_iterator const& l, spatial_query_iterator const& r)
    {
        return !(l.m_visitor == r.m_visitor);
    }

    friend bool operator!=(spatial_query_iterator const& l, end_query_iterator<Value, Allocators>)
    {
        return !l.m_visitor.is_end();
    }

    friend bool operator!=(end_query_iterator<Value, Allocators>, spatial_query_iterator const& r)
    {
        return !r.m_visitor.is_end();
    }
    
private:
    visitor_type m_visitor;
};

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators, typename Predicates, unsigned NearestPredicateIndex>
class distance_query_iterator
{
    typedef visitors::distance_query_incremental<Value, Options, Translator, Box, Allocators, Predicates, NearestPredicateIndex> visitor_type;
    typedef typename visitor_type::node_pointer node_pointer;

public:
    typedef std::input_iterator_tag iterator_category;
    typedef Value value_type;
    typedef typename Allocators::const_reference reference;
    typedef typename Allocators::difference_type difference_type;
    typedef typename Allocators::const_pointer pointer;

    inline distance_query_iterator(Translator const& t, Predicates const& p)
        : m_visitor(t, p)
    {}

    inline distance_query_iterator(node_pointer root, Translator const& t, Predicates const& p)
        : m_visitor(t, p)
    {
        detail::rtree::apply_visitor(m_visitor, *root);
        m_visitor.increment();
    }

    reference operator*() const
    {
        return m_visitor.dereference();
    }

    const value_type * operator->() const
    {
        return boost::addressof(m_visitor.dereference());
    }

    distance_query_iterator & operator++()
    {
        m_visitor.increment();
        return *this;
    }

    distance_query_iterator operator++(int)
    {
        distance_query_iterator temp = *this;
        this->operator++();
        return temp;
    }

    friend bool operator==(distance_query_iterator const& l, distance_query_iterator const& r)
    {
        return l.m_visitor == r.m_visitor;
    }

    friend bool operator==(distance_query_iterator const& l, end_query_iterator<Value, Allocators>)
    {
        return l.m_visitor.is_end();
    }

    friend bool operator==(end_query_iterator<Value, Allocators>, distance_query_iterator const& r)
    {
        return r.m_visitor.is_end();
    }

    friend bool operator!=(distance_query_iterator const& l, distance_query_iterator const& r)
    {
        return !(l.m_visitor == r.m_visitor);
    }

    friend bool operator!=(distance_query_iterator const& l, end_query_iterator<Value, Allocators>)
    {
        return !l.m_visitor.is_end();
    }

    friend bool operator!=(end_query_iterator<Value, Allocators>, distance_query_iterator const& r)
    {
        return !r.m_visitor.is_end();
    }

private:
    visitor_type m_visitor;
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUERY_ITERATORS_HPP
