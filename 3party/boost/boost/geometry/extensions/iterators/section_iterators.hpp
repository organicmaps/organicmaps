// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SECTION_ITERATORS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SECTION_ITERATORS_HPP

// 24-04-2010, Moved to extensions/iterators
//             because it was not yet used in the part.

// FILE WILL BE SPLITTED


#include <iterator>

#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/iterators/base.hpp>
#include <boost/geometry/algorithms/overlaps.hpp>


namespace boost { namespace geometry
{


namespace detail
{

    template <size_t D, typename P, typename B>
    inline bool exceeding(short int dir, P const& point, B const& box)
    {
        return (dir == 1  && get<D>(point) > get<1, D>(box))
            || (dir == -1 && get<D>(point) < get<0, D>(box));
    }

    template <size_t D, typename P, typename B>
    inline bool preceding(short int dir, P const& point, B const& box)
    {
        return (dir == 1  && get<D>(point) < get<0, D>(box))
            || (dir == -1 && get<D>(point) > get<1, D>(box));
    }
}


// Iterator walking through ring/sections, delivering only those points of the ring
// which are inside the specified box (using the sections)
template<typename G, typename S, typename B, size_t D>
struct section_iterator : public detail::iterators::iterator_base<
                    section_iterator<G, S, B, D>,
                    typename boost::range_iterator<G const>::type
                >
{
    friend class boost::iterator_core_access;

    inline section_iterator(G const& ring, S const& sections, B const& box)
        : m_ring(ring)
        , m_sections(sections)
        , m_box(box)
        , m_section_iterator(boost::begin(m_sections))
    {
        stay_within_box();
    }


private :

    inline void increment()
    {
        (this->base_reference())++;

        // If end or exceeding specified box, go to next section
        if (this->base() == m_end
                || detail::exceeding<D>(m_section_iterator->directions[0], *this->base(), m_box))
        {
            m_section_iterator++;
            stay_within_box();
        }

    }

    // Check if iterator is still in box and if not, go to the next section and advance until it is in box
    void stay_within_box()
    {
        // Find section having overlap with specified box
        while (m_section_iterator != boost::end(m_sections)
                && ! overlaps(m_section_iterator->bounding_box, m_box))
        {
            m_section_iterator++;
        }
        if (m_section_iterator != boost::end(m_sections))
        {
            this->base_reference() = boost::begin(m_ring) + m_section_iterator->begin_index;
            m_end = boost::begin(m_ring) + m_section_iterator->end_index + 1;

            // While not yet at box, advance
            while(this->base() != m_end
                && detail::preceding<D>(m_section_iterator->directions[0], *this->base(), m_box))
            {
                ++(this->base_reference());
            }

            if (this->base() == m_end)
            {
                // This should actually not occur because of bbox check, but to be sure
                m_section_iterator++;
                stay_within_box();
            }
        }
        else
        {
            this->base_reference() = boost::end(m_ring);
            m_end = boost::end(m_ring);
        }
    }


    typedef typename boost::range_iterator<G const>::type IT;
    typedef typename boost::range_iterator<S const>::type SIT;

    G const& m_ring;
    S const& m_sections;
    B const& m_box;

    IT m_end;
    SIT m_section_iterator;
};


// Iterator walking through ring/sections, delivering only those points of the ring
// which are inside the specified box (using the sections)
template<typename G, typename SEC, typename B, size_t D>
struct one_section_segment_iterator : public detail::iterators::iterator_base<
                one_section_segment_iterator<G, SEC, B, D>
                , typename boost::range_iterator<G const>::type>
{
    friend class boost::iterator_core_access;
    typedef typename boost::range_iterator<G const>::type normal_iterator;

    inline one_section_segment_iterator(G const& ring, SEC const& section, B const& box)
        : m_box(&box)
        , m_dir(section.directions[0])
    {
        init(section, ring);
    }

    inline one_section_segment_iterator(normal_iterator end)
        : m_section_end(end)
        , m_ring_end(end)
        , m_box(NULL)
        , m_dir(0)
    {
        this->base_reference() = end;
    }


private :

    inline void increment()
    {
        m_previous = (this->base_reference())++;

        if (this->base() == m_section_end
            || detail::exceeding<D>(m_dir, *m_previous, *m_box))
        {
            this->base_reference() = m_ring_end;
        }
    }

    // Check if iterator is still in box and if not, go to the next section and advance until it is in box
    void init(SEC const& section, G const& ring)
    {
        //this->base_reference();
        m_section_end = boost::begin(ring) + section.end_index + 1;
        m_ring_end = boost::end(ring);

        this->base_reference() = boost::begin(ring) + section.begin_index;

        /* Performance, TO BE CHECKED!
        normal_iterator next = boost::begin(ring) + section.begin_index;
        if (next != m_section_end && next != m_ring_end)
        {

            // While (not end and) not yet at box, advance
            normal_iterator it = next++;
            while(next != m_section_end && next != m_ring_end
                    && detail::preceding<D>(m_dir, *next, *m_box))
            {
                it = next++;
            }


            if (it == m_section_end)
            {
                this->base_reference() = m_ring_end;
            }
            else
            {
                this->base_reference() = it;
            }
        }
        else
        {
            this->base_reference() = m_ring_end;
        }
        */

        m_previous = this->base();
    }


    const B* m_box;
    short int m_dir;

    normal_iterator m_previous;
    normal_iterator m_section_end;
    normal_iterator m_ring_end;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ITERATORS_SECTION_ITERATORS_HPP
