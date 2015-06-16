// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKB_UTILITY_HPP
#define BOOST_GEOMETRY_IO_WKB_UTILITY_HPP

#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_convertible.hpp>

#include <boost/geometry/core/assert.hpp>

namespace boost { namespace geometry
{

// TODO: Waiting for errors handling design, eventually return bool
// may be replaced to throw exception.

template <typename OutputIterator>
bool hex2wkb(std::string const& hex, OutputIterator bytes)
{
    // Bytes can be only written to output iterator.
    BOOST_STATIC_ASSERT((boost::is_convertible<
        typename std::iterator_traits<OutputIterator>::iterator_category,
        const std::output_iterator_tag&>::value));

    std::string::size_type const byte_size = 2;
    if (0 != hex.size() % byte_size)
    {
        return false;
    }

    std::string::size_type const size = hex.size() / byte_size;
    for (std::string::size_type i = 0; i < size; ++i)
    {
        // TODO: This is confirmed performance killer - to be replaced with static char-to-byte map --mloskot
        std::istringstream iss(hex.substr(i * byte_size, byte_size));
        unsigned int byte(0);
        if (!(iss >> std::hex >> byte))
        {
            return false;
        }
        *bytes = static_cast<boost::uint8_t>(byte);
        ++bytes;
    }

    return true;
}

template <typename Iterator>
bool wkb2hex(Iterator begin, Iterator end, std::string& hex)
{
    // Stream of bytes can only be passed using random access iterator.
    BOOST_STATIC_ASSERT((boost::is_convertible<
        typename std::iterator_traits<Iterator>::iterator_category,
        const std::random_access_iterator_tag&>::value));

    const char hexalpha[] = "0123456789ABCDEF";
    char hexbyte[3] = { 0 };
    std::ostringstream oss;

    Iterator it = begin;
    while (it != end)
    {
        boost::uint8_t byte = static_cast<boost::uint8_t>(*it);
        hexbyte[0] = hexalpha[(byte >> 4) & 0xf];
        hexbyte[1] = hexalpha[byte & 0xf];
        hexbyte[2] = '\0';
        oss << std::setw(2) << hexbyte;
        ++it;
    }

    // TODO: Binary streams can be big.
    // Does it make sense to request stream buffer of proper (large) size or
    // use incremental appends within while-loop?
    hex = oss.str();

    // Poor-man validation, no performance penalty expected
    // because begin/end always are random access iterators.
    typename std::iterator_traits<Iterator>::difference_type
        diff = std::distance(begin, end);
    BOOST_GEOMETRY_ASSERT(diff > 0);
    return hex.size() == 2 * std::string::size_type(diff);
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_WKB_UTILITY_HPP
