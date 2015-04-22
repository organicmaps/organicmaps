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

#ifndef QUERY_NODE_HPP
#define QUERY_NODE_HPP

#include "../typedefs.h"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>

#include <limits>

struct QueryNode
{
    using key_type = NodeID; // type of NodeID
    using value_type = int;  // type of lat,lons

    explicit QueryNode(int lat, int lon, NodeID node_id) : lat(lat), lon(lon), node_id(node_id) {}
    QueryNode()
        : lat(std::numeric_limits<int>::max()), lon(std::numeric_limits<int>::max()),
          node_id(std::numeric_limits<unsigned>::max())
    {
    }

    int lat;
    int lon;
    NodeID node_id;

    static QueryNode min_value()
    {
        return QueryNode(static_cast<int>(-90 * COORDINATE_PRECISION),
                         static_cast<int>(-180 * COORDINATE_PRECISION),
                         std::numeric_limits<NodeID>::min());
    }

    static QueryNode max_value()
    {
        return QueryNode(static_cast<int>(90 * COORDINATE_PRECISION),
                         static_cast<int>(180 * COORDINATE_PRECISION),
                         std::numeric_limits<NodeID>::max());
    }

    value_type operator[](const std::size_t n) const
    {
        switch (n)
        {
        case 1:
            return lat;
        case 0:
            return lon;
        default:
            break;
        }
        BOOST_ASSERT_MSG(false, "should not happen");
        return std::numeric_limits<int>::lowest();
    }
};

#endif // QUERY_NODE_HPP
