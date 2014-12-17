/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef CONTAINER_HPP_
#define CONTAINER_HPP_

#include <algorithm>
#include <iterator>
#include <vector>

namespace osrm
{
template <typename T> void sort_unique_resize(std::vector<T> &vector)
{
    std::sort(vector.begin(), vector.end());
    const auto number_of_unique_elements = std::unique(vector.begin(), vector.end()) - vector.begin();
    vector.resize(number_of_unique_elements);
}

// template <typename T> void sort_unique_resize_shrink_vector(std::vector<T> &vector)
// {
//     sort_unique_resize(vector);
//     vector.shrink_to_fit();
// }

// template <typename T> void remove_consecutive_duplicates_from_vector(std::vector<T> &vector)
// {
//     const auto number_of_unique_elements = std::unique(vector.begin(), vector.end()) - vector.begin();
//     vector.resize(number_of_unique_elements);
// }

template <typename ForwardIterator, typename Function>
Function for_each_pair(ForwardIterator begin, ForwardIterator end, Function function)
{
    if (begin == end)
    {
        return function;
    }

    auto next = begin;
    next = std::next(next);

    while (next != end)
    {
        function(*begin, *next);
        begin = std::next(begin); next = std::next(next);
    }
    return function;
}
}
#endif /* CONTAINER_HPP_ */
