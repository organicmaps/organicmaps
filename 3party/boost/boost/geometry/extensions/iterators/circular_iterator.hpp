// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ITERATORS_CIRCULAR_ITERATOR_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ITERATORS_CIRCULAR_ITERATOR_HPP

#include <boost/iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>

#include <boost/geometry/iterators/base.hpp>


namespace boost { namespace geometry
{

/*!
    \brief Iterator which goes circular through a range, starting at a point, ending at that point
    \tparam Iterator iterator on which this class is based on
    \ingroup iterators
*/
template <typename Iterator>
struct circular_iterator :
    public detail::iterators::iterator_base
    <
        circular_iterator<Iterator>,
        Iterator
    >
{
    friend class boost::iterator_core_access;

    explicit inline circular_iterator(Iterator begin, Iterator end, Iterator start)
        : m_begin(begin)
        , m_end(end)
        , m_start(start)
    {
        this->base_reference() = start;
    }

    // Constructor to indicate the end of a range, to enable e.g. std::copy
    explicit inline circular_iterator(Iterator end)
        : m_begin(end)
        , m_end(end)
        , m_start(end)
    {
        this->base_reference() = end;
    }

    /// Navigate to a certain position, should be in [start .. end], it at end
    /// it will circle again.
    inline void moveto(Iterator it)
    {
        this->base_reference() = it;
        check_end();
    }

private:

    inline void increment()
    {
        if (this->base() != m_end)
        {
            (this->base_reference())++;
            check_end();
        }
    }
    inline void decrement()
    {
        if (this->base() != m_end)
        {
            // If at begin, go back to end (assumed this is possible...)
            if (this->base() == m_begin)
            {
                this->base_reference() = this->m_end;
            }

            // Decrement
            (this->base_reference())--;

            // If really back at start, go to end == end of iteration
            if (this->base() == m_start)
            {
                this->base_reference() = this->m_end;
            }
        }
    }


    inline void check_end()
    {
        if (this->base() == this->m_end)
        {
            this->base_reference() = this->m_begin;
        }

        if (this->base() == m_start)
        {
            this->base_reference() = this->m_end;
        }
    }

    Iterator m_begin;
    Iterator m_end;
    Iterator m_start;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ITERATORS_CIRCULAR_ITERATOR_HPP
