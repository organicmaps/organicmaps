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

#ifndef ORIGINAL_EDGE_DATA_H
#define ORIGINAL_EDGE_DATA_H

#include "TurnInstructions.h"
#include "../typedefs.h"

#include <limits>

struct OriginalEdgeData
{
    explicit OriginalEdgeData(NodeID via_node,
                              unsigned name_id,
                              TurnInstruction turn_instruction,
                              bool compressed_geometry)
        : via_node(via_node), name_id(name_id), turn_instruction(turn_instruction),
          compressed_geometry(compressed_geometry)
    {
    }

    OriginalEdgeData()
        : via_node(std::numeric_limits<unsigned>::max()),
          name_id(std::numeric_limits<unsigned>::max()),
          turn_instruction(TurnInstruction::NoTurn), compressed_geometry(false)
    {
    }

    NodeID via_node;
    unsigned name_id;
    TurnInstruction turn_instruction;
    bool compressed_geometry;
};

#endif // ORIGINAL_EDGE_DATA_H
