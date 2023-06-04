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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <limits>

// Necessary workaround for Windows as VS doesn't implement C99
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

using NodeID = unsigned int;
using EdgeID = unsigned int;
using EdgeWeight = int;

static const NodeID SPECIAL_NODEID = std::numeric_limits<unsigned>::max();
static const EdgeID SPECIAL_EDGEID = std::numeric_limits<unsigned>::max();
static const unsigned INVALID_NAMEID = std::numeric_limits<unsigned>::max();
static const EdgeWeight INVALID_EDGE_WEIGHT = std::numeric_limits<int>::max();

#endif /* TYPEDEFS_H */
