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

#ifndef GEOMETRY_COMPRESSOR_HPP_
#define GEOMETRY_COMPRESSOR_HPP_

#include "../typedefs.h"

#include <unordered_map>

#include <string>
#include <vector>

class GeometryCompressor
{
  public:
    using CompressedNode = std::pair<NodeID, EdgeWeight>;

    GeometryCompressor();
    void CompressEdge(const EdgeID surviving_edge_id,
                      const EdgeID removed_edge_id,
                      const NodeID via_node_id,
                      const NodeID target_node,
                      const EdgeWeight weight1,
                      const EdgeWeight weight2);

    bool HasEntryForID(const EdgeID edge_id) const;
    void PrintStatistics() const;
    void SerializeInternalVector(const std::string &path) const;
    unsigned GetPositionForID(const EdgeID edge_id) const;
    const std::vector<GeometryCompressor::CompressedNode> &
    GetBucketReference(const EdgeID edge_id) const;
    NodeID GetFirstNodeIDOfBucket(const EdgeID edge_id) const;
    NodeID GetLastNodeIDOfBucket(const EdgeID edge_id) const;

  private:
    int free_list_maximum = 0;
    
    void IncreaseFreeList();
    std::vector<std::vector<CompressedNode>> m_compressed_geometries;
    std::vector<unsigned> m_free_list;
    std::unordered_map<EdgeID, unsigned> m_edge_id_to_list_index_map;
};

#endif // GEOMETRY_COMPRESSOR_HPP_
