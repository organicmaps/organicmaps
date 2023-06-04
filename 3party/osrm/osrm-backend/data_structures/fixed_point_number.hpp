/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef FIXED_POINT_NUMBER_HPP
#define FIXED_POINT_NUMBER_HPP

#include <cmath>
#include <cstdint>

#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

namespace osrm
{

// implements an binary based fixed point number type
template <unsigned FractionalBitSize,
          bool use_64_bits = false,
          bool is_unsigned = false,
          bool truncate_results = false>
class FixedPointNumber
{
    static_assert(FractionalBitSize > 0, "FractionalBitSize must be greater than 0");
    static_assert(FractionalBitSize <= 32, "FractionalBitSize must at most 32");

    typename std::conditional<use_64_bits, int64_t, int32_t>::type m_fixed_point_state;
    constexpr static const decltype(m_fixed_point_state) PRECISION = 1 << FractionalBitSize;

    // state signage encapsulates whether the state should either represent a
    // signed or an unsigned floating point number
    using state_signage =
        typename std::conditional<is_unsigned,
                                  typename std::make_unsigned<decltype(m_fixed_point_state)>::type,
                                  decltype(m_fixed_point_state)>::type;

  public:
    FixedPointNumber() : m_fixed_point_state(0) {}

    // the type is either initialized with a floating point value or an
    // integral state. Anything else will throw at compile-time.
    template <class T>
    constexpr FixedPointNumber(const T &&input) noexcept
        : m_fixed_point_state(static_cast<decltype(m_fixed_point_state)>(
              std::round(std::forward<const T>(input) * PRECISION)))
    {
        static_assert(
            std::is_floating_point<T>::value || std::is_integral<T>::value,
            "FixedPointNumber needs to be initialized with floating point or integral value");
    }

    // get max value
    template <typename T,
              typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    constexpr static auto max() noexcept -> T
    {
        return static_cast<T>(std::numeric_limits<state_signage>::max()) / PRECISION;
    }

    // get min value
    template <typename T,
              typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    constexpr static auto min() noexcept -> T
    {
        return static_cast<T>(1) / PRECISION;
    }

    // get lowest value
    template <typename T,
              typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    constexpr static auto lowest() noexcept -> T
    {
        return static_cast<T>(std::numeric_limits<state_signage>::min()) / PRECISION;
    }

    // cast to floating point type T, return value
    template <typename T,
              typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    explicit operator const T() const noexcept
    {
        // casts to external type (signed or unsigned) and then to float
        return static_cast<T>(static_cast<state_signage>(m_fixed_point_state)) / PRECISION;
    }

    // warn about cast to integral type T, its disabled for good reason
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    explicit operator T() const
    {
        static_assert(std::is_integral<T>::value,
                      "casts to integral types have been disabled on purpose");
    }

    // compare, ie. sort fixed-point numbers
    bool operator<(const FixedPointNumber &other) const noexcept
    {
        return m_fixed_point_state < other.m_fixed_point_state;
    }

    // equality, ie. sort fixed-point numbers
    bool operator==(const FixedPointNumber &other) const noexcept
    {
        return m_fixed_point_state == other.m_fixed_point_state;
    }

    bool operator!=(const FixedPointNumber &other) const { return !(*this == other); }
    bool operator>(const FixedPointNumber &other) const { return other < *this; }
    bool operator<=(const FixedPointNumber &other) const { return !(other < *this); }
    bool operator>=(const FixedPointNumber &other) const { return !(*this < other); }

    // arithmetic operators
    FixedPointNumber operator+(const FixedPointNumber &other) const noexcept
    {
        FixedPointNumber tmp = *this;
        tmp.m_fixed_point_state += other.m_fixed_point_state;
        return tmp;
    }

    FixedPointNumber &operator+=(const FixedPointNumber &other) noexcept
    {
        this->m_fixed_point_state += other.m_fixed_point_state;
        return *this;
    }

    FixedPointNumber operator-(const FixedPointNumber &other) const noexcept
    {
        FixedPointNumber tmp = *this;
        tmp.m_fixed_point_state -= other.m_fixed_point_state;
        return tmp;
    }

    FixedPointNumber &operator-=(const FixedPointNumber &other) noexcept
    {
        this->m_fixed_point_state -= other.m_fixed_point_state;
        return *this;
    }

    FixedPointNumber operator*(const FixedPointNumber &other) const noexcept
    {
        int64_t temp = this->m_fixed_point_state;
        temp *= other.m_fixed_point_state;

        // rounding!
        if (!truncate_results)
        {
            temp = temp + ((temp & 1 << (FractionalBitSize - 1)) << 1);
        }
        temp >>= FractionalBitSize;
        FixedPointNumber tmp;
        tmp.m_fixed_point_state = static_cast<decltype(m_fixed_point_state)>(temp);
        return tmp;
    }

    FixedPointNumber &operator*=(const FixedPointNumber &other) noexcept
    {
        int64_t temp = this->m_fixed_point_state;
        temp *= other.m_fixed_point_state;

        // rounding!
        if (!truncate_results)
        {
            temp = temp + ((temp & 1 << (FractionalBitSize - 1)) << 1);
        }
        temp >>= FractionalBitSize;
        this->m_fixed_point_state = static_cast<decltype(m_fixed_point_state)>(temp);
        return *this;
    }

    FixedPointNumber operator/(const FixedPointNumber &other) const noexcept
    {
        int64_t temp = this->m_fixed_point_state;
        temp <<= FractionalBitSize;
        temp /= static_cast<int64_t>(other.m_fixed_point_state);
        FixedPointNumber tmp;
        tmp.m_fixed_point_state = static_cast<decltype(m_fixed_point_state)>(temp);
        return tmp;
    }

    FixedPointNumber &operator/=(const FixedPointNumber &other) noexcept
    {
        int64_t temp = this->m_fixed_point_state;
        temp <<= FractionalBitSize;
        temp /= static_cast<int64_t>(other.m_fixed_point_state);
        FixedPointNumber tmp;
        this->m_fixed_point_state = static_cast<decltype(m_fixed_point_state)>(temp);
        return *this;
    }
};

static_assert(4 == sizeof(FixedPointNumber<1>), "FP19 has wrong size != 4");
}
#endif // FIXED_POINT_NUMBER_HPP
