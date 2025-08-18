/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef INTEGER_RANGE_HPP
#define INTEGER_RANGE_HPP

#include <type_traits>

namespace osrm
{

template <typename Integer> class range
{
  private:
    Integer last;
    Integer iter;

  public:
    range(Integer start, Integer end) noexcept : last(end), iter(start)
    {
        static_assert(std::is_integral<Integer>::value, "range type must be integral");
    }

    // Iterable functions
    const range &begin() const noexcept { return *this; }
    const range &end() const noexcept { return *this; }
    Integer front() const noexcept { return iter; }
    Integer back() const noexcept { return last - 1; }
    Integer size() const noexcept { return last - iter; }

    // Iterator functions
    bool operator!=(const range &) const noexcept { return iter < last; }
    void operator++() noexcept { ++iter; }
    Integer operator*() const noexcept { return iter; }
};

// convenience function to construct an integer range with type deduction
template <typename Integer>
range<Integer>
irange(const Integer first,
       const Integer last,
       typename std::enable_if<std::is_integral<Integer>::value>::type * = 0) noexcept
{
    return range<Integer>(first, last);
}
}

#endif // INTEGER_RANGE_HPP
