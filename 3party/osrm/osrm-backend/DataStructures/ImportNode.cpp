/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

#include "ImportNode.h"

#include <limits>

ExternalMemoryNode::ExternalMemoryNode(
    int lat, int lon, unsigned int node_id, bool bollard, bool traffic_light)
    : NodeInfo(lat, lon, node_id), bollard(bollard), trafficLight(traffic_light)
{
}

ExternalMemoryNode::ExternalMemoryNode() : bollard(false), trafficLight(false)
{
}

ExternalMemoryNode ExternalMemoryNode::min_value()
{
    return ExternalMemoryNode(0, 0, 0, false, false);
}

ExternalMemoryNode ExternalMemoryNode::max_value()
{
    return ExternalMemoryNode(std::numeric_limits<int>::max(),
                              std::numeric_limits<int>::max(),
                              std::numeric_limits<unsigned>::max(),
                              false,
                              false);
}

void ImportNode::Clear()
{
    keyVals.Clear();
    lat = 0;
    lon = 0;
    node_id = 0;
    bollard = false;
    trafficLight = false;
}
