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

#ifndef LOWER_BOUND_HPP
#define LOWER_BOUND_HPP

#include <functional>
#include <limits>
#include <queue>
#include <type_traits>

// max pq holds k elements
// insert if key is smaller than max
// if size > k then remove element
// get() always yields a bound to the k smallest element in the stream

template <typename key_type> class upper_bound
{
  private:
    using parameter_type =
        typename std::conditional<std::is_fundamental<key_type>::value, key_type, key_type &>::type;

  public:
    upper_bound() = delete;
    upper_bound(std::size_t size) : size(size) {}

    key_type get() const
    {
        if (queue.size() < size)
        {
            return std::numeric_limits<key_type>::max();
        }
        return queue.top();
    }

    void insert(const parameter_type key)
    {
        if (key < get())
        {
            queue.emplace(key);
            while (queue.size() > size)
            {
                queue.pop();
            }
        }
    }

  private:
    std::priority_queue<key_type, std::vector<key_type>, std::less<key_type>> queue;
    const std::size_t size;
};

#endif // LOWER_BOUND_HPP
