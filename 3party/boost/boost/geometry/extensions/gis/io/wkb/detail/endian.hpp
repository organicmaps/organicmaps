// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Load/Store values from/to stream of bytes across different endianness.

// Original design of unrolled_byte_loops templates based on
// endian utility library from Boost C++ Libraries,
// source: boost/spirit/home/support/detail/integer/endian.hpp
// Copyright Darin Adler 2000
// Copyright Beman Dawes 2006, 2009
// Distributed under the Boost Software License, Version 1.0.

#ifndef BOOST_GEOMETRY_DETAIL_ENDIAN_HPP
#define BOOST_GEOMETRY_DETAIL_ENDIAN_HPP

#include <cassert>
#include <climits>
#include <cstring>
#include <cstddef>

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/endian.hpp>
#include <boost/type_traits/is_signed.hpp>

#if CHAR_BIT != 8
#error Platforms with CHAR_BIT != 8 are not supported
#endif

// TODO: mloskot - add static asserts to validate compile-time pre-conditions

namespace boost { namespace geometry
{

namespace detail { namespace endian
{

// Endianness tag used to indicate load/store directoin

struct big_endian_tag {};
struct little_endian_tag {};

#ifdef BOOST_BIG_ENDIAN
typedef big_endian_tag native_endian_tag;
#else
typedef little_endian_tag native_endian_tag;
#endif

// Unrolled loops for loading and storing streams of bytes.

template <typename T, std::size_t N, bool Sign = boost::is_signed<T>::value>
struct unrolled_byte_loops
{
    typedef unrolled_byte_loops<T, N - 1, Sign> next;

    template <typename Iterator>
    static T load_forward(Iterator& bytes)
    {
        T const value = *bytes;
        ++bytes;
        return value | (next::load_forward(bytes) << 8);
    }

    template <typename Iterator>
    static T load_backward(Iterator& bytes)
    {
        T const value = *(bytes - 1);
        --bytes;
        return value | (next::load_backward(bytes) << 8);
    }

    template <typename Iterator>
    static void store_forward(Iterator& bytes, T value)
    {
        *bytes = static_cast<char>(value);
        next::store_forward(++bytes, value >> 8);
    }

    template <typename Iterator>
    static void store_backward(Iterator& bytes, T value)
    {
        *(bytes - 1) = static_cast<char>(value);
        next::store_backward(--bytes, value >> 8);
    }
};

template <typename T>
struct unrolled_byte_loops<T, 1, false>
{
    template <typename Iterator>
    static T load_forward(Iterator& bytes)
    {
        return *bytes;
    }

    template <typename Iterator>
    static T load_backward(Iterator& bytes)
    {
        return *(bytes - 1);
    }

    template <typename Iterator>
    static void store_forward(Iterator& bytes, T value)
    {
        // typename Iterator::value_type
        *bytes = static_cast<char>(value);
    }

    template <typename Iterator>
    static void store_backward(Iterator& bytes, T value)
    {
        *(bytes - 1) = static_cast<char>(value);
    }
};

template <typename T>
struct unrolled_byte_loops<T, 1, true>
{
    template <typename Iterator>
    static T load_forward(Iterator& bytes)
    {
        return *reinterpret_cast<const signed char*>(&*bytes);
    }

    template <typename Iterator>
    static T load_backward(Iterator& bytes)
    {
        return *reinterpret_cast<const signed char*>(&*(bytes - 1));
    }

    template <typename Iterator>
    static void store_forward(Iterator& bytes, T value)
    {
        BOOST_STATIC_ASSERT((boost::is_signed<typename Iterator::value_type>::value));

        *bytes = static_cast<typename Iterator::value_type>(value);
    }

    template <typename Iterator>
    static void store_backward(Iterator& bytes, T value)
    {
        BOOST_STATIC_ASSERT((boost::is_signed<typename Iterator::value_type>::value));

        *(bytes - 1) = static_cast<typename Iterator::value_type>(value);
    }
};

// load/store operation dispatch
// E, E - source and target endianness is the same
// E1, E2 - source and target endianness is different (big-endian <-> little-endian)

template <typename T, std::size_t N, typename Iterator, typename E>
T load_dispatch(Iterator& bytes, E, E)
{
    return unrolled_byte_loops<T, N>::load_forward(bytes);
}

template <typename T, std::size_t N, typename Iterator, typename E1, typename E2>
T load_dispatch(Iterator& bytes, E1, E2)
{
    std::advance(bytes, N);
    return unrolled_byte_loops<T, N>::load_backward(bytes);
}

template <typename T, std::size_t N, typename Iterator, typename E>
void store_dispatch(Iterator& bytes, T value, E, E)
{
    return unrolled_byte_loops<T, N>::store_forward(bytes, value);
}

template <typename T, std::size_t N, typename Iterator, typename E1, typename E2>
void store_dispatch(Iterator& bytes, T value, E1, E2)
{
    std::advance(bytes, N);
    return unrolled_byte_loops<T, N>::store_backward(bytes, value);
}

// numeric value holder for load/store operation

template <typename T>
struct endian_value_base
{
    typedef T value_type;
    typedef native_endian_tag endian_type;

    endian_value_base() : value(T()) {}
    explicit endian_value_base(T value) : value(value) {}

    operator T() const
    {
        return value;
    }

protected:
    T value;
};

template <typename T, std::size_t N = sizeof(T)>
struct endian_value : public endian_value_base<T>
{
    typedef endian_value_base<T> base;

    endian_value() {}
    explicit endian_value(T value) : base(value) {}

    template <typename E, typename Iterator>
    void load(Iterator bytes)
    {
        base::value = load_dispatch<T, N>(bytes, typename base::endian_type(), E());
    }

    template <typename E, typename Iterator>
    void store(Iterator bytes)
    {
        store_dispatch<T, N>(bytes, base::value, typename base::endian_type(), E());
    }
};

template <>
struct endian_value<double, 8> : public endian_value_base<double>
{
    typedef endian_value_base<double> base;

    endian_value() {}
    explicit endian_value(double value) : base(value) {}

    template <typename E, typename Iterator>
    void load(Iterator bytes)
    {
        endian_value<boost::uint64_t, 8> raw;
        raw.load<E>(bytes);

        double& target_value = base::value;
        std::memcpy(&target_value, &raw, sizeof(double));
    }

    template <typename E, typename Iterator>
    void store(Iterator bytes)
    {
        boost::uint64_t raw;
        double const& source_value = base::value;
        std::memcpy(&raw, &source_value, sizeof(boost::uint64_t));

        store_dispatch
            <
            boost::uint64_t,
            sizeof(boost::uint64_t)
            >(bytes, raw, typename base::endian_type(), E());
    }
};

}} // namespace detail::endian
}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_DETAIL_ENDIAN_HPP

