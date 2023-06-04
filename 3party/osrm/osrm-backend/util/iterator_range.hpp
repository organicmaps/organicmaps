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

#ifndef ITERATOR_RANGE_HPP
#define ITERATOR_RANGE_HPP

namespace osrm
{
template <typename Iterator> class iter_range
{
  public:
    iter_range(Iterator begin, Iterator end) noexcept : begin_(begin), end_(end) {}

    Iterator begin() const noexcept { return begin_; }
    Iterator end() const noexcept { return end_; }

  private:
    Iterator begin_;
    Iterator end_;
};

// Convenience functions for template parameter inference,
// akin to std::make_pair.

template <typename Iterator>
iter_range<Iterator> integer_range(Iterator begin, Iterator end) noexcept
{
    return iter_range<Iterator>(begin, end);
}

template <typename Reversable>
iter_range<typename Reversable::reverse_iterator> reverse(Reversable *reversable) noexcept
{
    return iter_range<typename Reversable::reverse_iterator>(reversable->rbegin(),
                                                             reversable->rend());
}

template <typename ConstReversable>
iter_range<typename ConstReversable::const_reverse_iterator>
const_reverse(const ConstReversable *const_reversable) noexcept
{
    return iter_range<typename ConstReversable::const_reverse_iterator>(const_reversable->crbegin(),
                                                                        const_reversable->crend());
}
}

#endif // ITERATOR_RANGE_HPP
