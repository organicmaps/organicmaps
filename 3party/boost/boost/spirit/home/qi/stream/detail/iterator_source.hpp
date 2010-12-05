/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ITERATOR_ISTREAM_MAY_05_2007_0110PM)
#define BOOST_SPIRIT_ITERATOR_ISTREAM_MAY_05_2007_0110PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/iostreams/stream.hpp>
#include <boost/detail/iterator.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct iterator_source
    {
        typedef typename
            boost::detail::iterator_traits<Iterator>::value_type
        char_type;
        typedef boost::iostreams::source_tag category;

        iterator_source (Iterator& first_, Iterator const& last_)
          : first(first_), last(last_)
        {}

        // Read up to n characters from the input sequence into the buffer s,
        // returning the number of characters read, or -1 to indicate
        // end-of-sequence.
        std::streamsize read (char_type* s, std::streamsize n)
        {
            if (first == last)
                return -1;

            std::streamsize bytes_read = 0;
            while (n--) {
                *s = *first;
                ++s; ++bytes_read;
                if (++first == last)
                    break;
            }
            return bytes_read;
        }

        Iterator& first;
        Iterator const& last;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        iterator_source& operator= (iterator_source const&);
    };

}}}}

#endif
