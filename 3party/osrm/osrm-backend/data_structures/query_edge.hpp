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

#ifndef QUERYEDGE_HPP
#define QUERYEDGE_HPP

#include "../typedefs.h"

#include <tuple>

struct QueryEdge
{
    NodeID source;
    NodeID target;
    struct EdgeData
    {
        EdgeData() : id(0), shortcut(false), distance(0), forward(false), backward(false) {}

        template <class OtherT> EdgeData(const OtherT &other)
        {
            distance = other.distance;
            shortcut = other.shortcut;
            id = other.id;
            forward = other.forward;
            backward = other.backward;
        }
        NodeID id : 31;
        bool shortcut : 1;
        int distance : 30;
        bool forward : 1;
        bool backward : 1;
    } data;

    QueryEdge() : source(SPECIAL_NODEID), target(SPECIAL_NODEID) {}

    QueryEdge(NodeID source, NodeID target, EdgeData data)
        : source(source), target(target), data(data)
    {
    }

    bool operator<(const QueryEdge &rhs) const
    {
        return std::tie(source, target) < std::tie(rhs.source, rhs.target);
    }

    bool operator==(const QueryEdge &right) const
    {
        return (source == right.source && target == right.target &&
                data.distance == right.data.distance && data.shortcut == right.data.shortcut &&
                data.forward == right.data.forward && data.backward == right.data.backward &&
                data.id == right.data.id);
    }
};

#endif // QUERYEDGE_HPP
