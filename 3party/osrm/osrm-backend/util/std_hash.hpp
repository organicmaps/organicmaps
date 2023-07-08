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

#ifndef STD_HASH_HPP
#define STD_HASH_HPP

#include <functional>

// this is largely inspired by boost's hash combine as can be found in
// "The C++ Standard Library" 2nd Edition. Nicolai M. Josuttis. 2012.

namespace
{

template <typename T> void hash_combine(std::size_t &seed, const T &val)
{
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T> void hash_val(std::size_t &seed, const T &val) { hash_combine(seed, val); }

template <typename T, typename... Types>
void hash_val(std::size_t &seed, const T &val, const Types &... args)
{
    hash_combine(seed, val);
    hash_val(seed, args...);
}

template <typename... Types> std::size_t hash_val(const Types &... args)
{
    std::size_t seed = 0;
    hash_val(seed, args...);
    return seed;
}
}

namespace std
{
template <typename T1, typename T2> struct hash<std::pair<T1, T2>>
{
    size_t operator()(const std::pair<T1, T2> &pair) const
    {
        return hash_val(pair.first, pair.second);
    }
};
}

#endif // STD_HASH_HPP
