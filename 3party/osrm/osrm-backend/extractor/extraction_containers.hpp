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

#ifndef EXTRACTION_CONTAINERS_HPP
#define EXTRACTION_CONTAINERS_HPP

#include "internal_extractor_edge.hpp"
#include "first_and_last_segment_of_way.hpp"
#include "../data_structures/external_memory_node.hpp"
#include "../data_structures/restriction.hpp"
#include "../util/fingerprint.hpp"

#include <stxxl/vector>

class ExtractionContainers
{
#ifndef _MSC_VER
    constexpr static unsigned stxxl_memory =
        ((sizeof(std::size_t) == 4) ? std::numeric_limits<int>::max()
                                    : std::numeric_limits<unsigned>::max());
#else
    const static unsigned stxxl_memory = ((sizeof(std::size_t) == 4) ? INT_MAX : UINT_MAX);
#endif
  public:
    using STXXLNodeIDVector = stxxl::vector<NodeID>;
    using STXXLNodeVector = stxxl::vector<ExternalMemoryNode>;
    using STXXLEdgeVector = stxxl::vector<InternalExtractorEdge>;
    using STXXLStringVector = stxxl::vector<std::string>;
    using STXXLRestrictionsVector = stxxl::vector<InputRestrictionContainer>;
    using STXXLWayIDStartEndVector = stxxl::vector<FirstAndLastSegmentOfWay>;

    STXXLNodeIDVector used_node_id_list;
    STXXLNodeVector all_nodes_list;
    STXXLEdgeVector all_edges_list;
    STXXLStringVector name_list;
    STXXLRestrictionsVector restrictions_list;
    STXXLWayIDStartEndVector way_start_end_id_list;
    const FingerPrint fingerprint;

    ExtractionContainers();

    ~ExtractionContainers();

    void PrepareData(const std::string &output_file_name,
                     const std::string &restrictions_file_name);
};

#endif /* EXTRACTION_CONTAINERS_HPP */
