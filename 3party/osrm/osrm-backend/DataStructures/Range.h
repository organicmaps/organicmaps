/*

Copyright (c) 2013,2014, Project OSRM, Dennis Luxen, others
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

#ifndef RANGE_H
#define RANGE_H

#include <type_traits>

namespace osrm
{

template <typename Integer> class range
{
  private:
    Integer last;
    Integer iter;

  public:
    range(Integer start, Integer end) : last(end), iter(start)
    {
        static_assert(std::is_integral<Integer>::value, "range type must be integral");
    }

    // Iterable functions
    const range &begin() const { return *this; }
    const range &end() const { return *this; }
    Integer front() const { return iter; }
    Integer back() const { return last - 1; }

    // Iterator functions
    bool operator!=(const range &) const { return iter < last; }
    void operator++() { ++iter; }
    Integer operator*() const { return iter; }
};

// convenience function to construct an integer range with type deduction
template <typename Integer> range<Integer> irange(Integer first, Integer last)
{
    return range<Integer>(first, last);
}
}

#endif // RANGE_H
