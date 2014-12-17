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

#ifndef MAKE_UNIQUE_H_
#define MAKE_UNIQUE_H_

#include <cstdlib>
#include <memory>
#include <type_traits>

namespace osrm
{
// Taken from http://msdn.microsoft.com/en-us/library/dn439780.asp
// Note, that the snippet was broken there and needed minor massaging

// make_unique<T>
template <class T, class... Types> std::unique_ptr<T> make_unique(Types &&... Args)
{
    return (std::unique_ptr<T>(new T(std::forward<Types>(Args)...)));
}

// make_unique<T[]>
template <class T> std::unique_ptr<T[]> make_unique(std::size_t Size)
{
    return (std::unique_ptr<T>(new T[Size]()));
}

// make_unique<T[N]> disallowed
template <class T, class... Types>
typename std::enable_if<std::extent<T>::value != 0, void>::type make_unique(Types &&...) = delete;
}

#endif //MAKE_UNIQUE_H_
