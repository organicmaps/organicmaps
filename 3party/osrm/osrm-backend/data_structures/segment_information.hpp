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

#ifndef SEGMENT_INFORMATION_HPP
#define SEGMENT_INFORMATION_HPP

#include "turn_instructions.hpp"

#include "../data_structures/travel_mode.hpp"
#include "../typedefs.h"

#include <osrm/coordinate.hpp>

// Struct fits everything in one cache line
struct SegmentInformation
{
    FixedPointCoordinate location;
    NodeID name_id;
    EdgeWeight duration;
    float length;
    short bearing; // more than enough [0..3600] fits into 12 bits
    TurnInstruction turn_instruction;
    TravelMode travel_mode;
    bool necessary;
    bool is_via_location;

    explicit SegmentInformation(const FixedPointCoordinate &location,
                                const NodeID name_id,
                                const EdgeWeight duration,
                                const float length,
                                const TurnInstruction turn_instruction,
                                const bool necessary,
                                const bool is_via_location,
                                const TravelMode travel_mode)
        : location(location), name_id(name_id), duration(duration), length(length), bearing(0),
          turn_instruction(turn_instruction), travel_mode(travel_mode), necessary(necessary),
          is_via_location(is_via_location)
    {
    }

    explicit SegmentInformation(const FixedPointCoordinate &location,
                                const NodeID name_id,
                                const EdgeWeight duration,
                                const float length,
                                const TurnInstruction turn_instruction,
                                const TravelMode travel_mode)
        : location(location), name_id(name_id), duration(duration), length(length), bearing(0),
          turn_instruction(turn_instruction), travel_mode(travel_mode),
          necessary(turn_instruction != TurnInstruction::NoTurn), is_via_location(false)
    {
    }
};

#endif /* SEGMENT_INFORMATION_HPP */
