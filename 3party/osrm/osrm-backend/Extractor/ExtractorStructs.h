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

#ifndef EXTRACTORSTRUCTS_H_
#define EXTRACTORSTRUCTS_H_

#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../typedefs.h"

#include <limits>
#include <string>

struct ExtractorRelation
{
    ExtractorRelation() : type(unknown) {}
    enum
    { unknown = 0,
      ferry,
      turnRestriction } type;
    HashTable<std::string, std::string> keyVals;
};

struct WayIDStartAndEndEdge
{
    unsigned wayID;
    NodeID firstStart;
    NodeID firstTarget;
    NodeID lastStart;
    NodeID lastTarget;
    WayIDStartAndEndEdge()
        : wayID(std::numeric_limits<unsigned>::max()), firstStart(std::numeric_limits<unsigned>::max()), firstTarget(std::numeric_limits<unsigned>::max()), lastStart(std::numeric_limits<unsigned>::max()),
          lastTarget(std::numeric_limits<unsigned>::max())
    {
    }

    explicit WayIDStartAndEndEdge(unsigned w, NodeID fs, NodeID ft, NodeID ls, NodeID lt)
        : wayID(w), firstStart(fs), firstTarget(ft), lastStart(ls), lastTarget(lt)
    {
    }

    static WayIDStartAndEndEdge min_value()
    {
        return WayIDStartAndEndEdge((std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)());
    }
    static WayIDStartAndEndEdge max_value()
    {
        return WayIDStartAndEndEdge((std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)());
    }
};

struct CmpWayByID
{
    typedef WayIDStartAndEndEdge value_type;
    bool operator()(const WayIDStartAndEndEdge &a, const WayIDStartAndEndEdge &b) const
    {
        return a.wayID < b.wayID;
    }
    value_type max_value() { return WayIDStartAndEndEdge::max_value(); }
    value_type min_value() { return WayIDStartAndEndEdge::min_value(); }
};

struct Cmp
{
    typedef NodeID value_type;
    bool operator()(const NodeID left, const NodeID right) const { return left < right; }
    value_type max_value() { return 0xffffffff; }
    value_type min_value() { return 0x0; }
};

struct CmpNodeByID
{
    typedef ExternalMemoryNode value_type;
    bool operator()(const ExternalMemoryNode &left, const ExternalMemoryNode &right) const
    {
        return left.node_id < right.node_id;
    }
    value_type max_value() { return ExternalMemoryNode::max_value(); }
    value_type min_value() { return ExternalMemoryNode::min_value(); }
};

#endif /* EXTRACTORSTRUCTS_H_ */
