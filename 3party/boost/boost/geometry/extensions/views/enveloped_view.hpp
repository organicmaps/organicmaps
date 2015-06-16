// Boost.Range (aka GGL, Generic Range Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_VIEWS_ENVELOPED_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_ENVELOPED_VIEW_HPP


// Note the addition of this whole file was committed to SVN by accident,
// probably obsolete

#include <boost/mpl/if.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/ring_type.hpp>


namespace boost { namespace geometry
{




template <typename Range, typename Box, std::size_t Dimension>
class enveloped_view
{
public :
    typedef typename boost::range_iterator<Range const>::type const_iterator;
    typedef typename boost::range_iterator<Range>::type iterator;

    explicit enveloped_view(Range& range, Box const& box, int dir)
        : m_begin(boost::begin(range))
        , m_end(boost::end(range))
    {
        find_first(dir, m_begin, m_end, box);
        find_last(dir, m_begin, m_end, box);

        // Assignment of const iterator to iterator seems no problem,
        // at least not for MSVC and GCC
        m_const_begin = m_begin;
        m_const_end = m_end;
        // Otherwise: repeat
        //find_first(dir, m_const_begin, m_const_end);
        //find_last(dir, m_const_begin, m_const_end);
    }

    const_iterator begin() const { return m_const_begin; }
    const_iterator end() const { return m_const_end; }

    iterator begin() { return m_begin; }
    iterator end() { return m_end; }

private :
    const_iterator m_const_begin, m_const_end;
    iterator m_begin, m_end;

    template <typename Point>
    inline bool preceding(short int dir, Point const& point, Box const& box)
    {
        return (dir == 1  && get<Dimension>(point) < get<0, Dimension>(box))
            || (dir == -1 && get<Dimension>(point) > get<1, Dimension>(box));
    }

    template <typename Point>
    inline bool exceeding(short int dir, Point const& point, Box const& box)
    {
        return (dir == 1  && get<Dimension>(point) > get<1, Dimension>(box))
            || (dir == -1 && get<Dimension>(point) < get<0, Dimension>(box));
    }

    template <typename Iterator>
    void find_first(int dir, Iterator& begin, Iterator const end, Box const& box)
    {
        if (begin != end)
        {
            if (exceeding(dir, *begin, box))
            {
                // First obvious check
                begin = end;
                return;
            }

            iterator it = begin;
            iterator prev = it++;
            for(; it != end && preceding(dir, *it, box); ++it, ++prev) {}
            begin = prev;
        }
    }

    template <typename Iterator>
    void find_last(int dir, Iterator& begin, Iterator& end, Box const& box)
    {
        if (begin != end)
        {
            iterator it = begin;
            iterator prev = it++;
            for(; it != end && ! exceeding(dir, *prev, box); ++it, ++prev) {}
            if (it == end && prev != end && preceding(dir, *prev, box))
            {
                // Last obvious check (not done before to not refer to *(end-1))
                begin = end;
            }
            else
            {
                end = it;
            }
        }
    }

};



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_ENVELOPED_VIEW_HPP
