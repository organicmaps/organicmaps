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

#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include <algorithm>
#include <iterator>
#include <vector>

namespace osrm
{
namespace detail
{
// Culled by SFINAE if reserve does not exist or is not accessible
template <typename T>
constexpr auto has_resize_method(T &t) noexcept -> decltype(t.resize(0), bool())
{
    return true;
}

// Used as fallback when SFINAE culls the template method
constexpr bool has_resize_method(...) noexcept { return false; }
}

template <typename Container> void sort_unique_resize(Container &vector) noexcept
{
    std::sort(std::begin(vector), std::end(vector));
    const auto number_of_unique_elements =
        std::unique(std::begin(vector), std::end(vector)) - std::begin(vector);
    if (detail::has_resize_method(vector))
    {
        vector.resize(number_of_unique_elements);
    }
}

// template <typename T> inline void sort_unique_resize_shrink_vector(std::vector<T> &vector)
// {
//     sort_unique_resize(vector);
//     vector.shrink_to_fit();
// }

// template <typename T> inline void remove_consecutive_duplicates_from_vector(std::vector<T>
// &vector)
// {
//     const auto number_of_unique_elements = std::unique(vector.begin(), vector.end()) -
//     vector.begin();
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
        begin = std::next(begin);
        next = std::next(next);
    }
    return function;
}

template <class ContainerT, typename Function>
Function for_each_pair(ContainerT &container, Function function)
{
    return for_each_pair(std::begin(container), std::end(container), function);
}

template <class Container> void append_to_container(Container &&a) {}

template <class Container, typename T, typename... Args>
void append_to_container(Container &&container, T value, Args &&... args)
{
    container.emplace_back(value);
    append_to_container(std::forward<Container>(container), std::forward<Args>(args)...);
}

} // namespace osrm
#endif /* CONTAINER_HPP */
