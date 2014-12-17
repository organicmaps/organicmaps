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

#ifndef RESTRICTION_MAP_H_
#define RESTRICTION_MAP_H_

#include <memory>

#include "DynamicGraph.h"
#include "Restriction.h"
#include "NodeBasedGraph.h"
#include "../Util/StdHashExtensions.h"
#include "../typedefs.h"

#include <unordered_map>
#include <unordered_set>

struct RestrictionSource
{
    NodeID start_node;
    NodeID via_node;

    RestrictionSource(NodeID start, NodeID via) : start_node(start), via_node(via)
    {
    }

    friend inline bool operator==(const RestrictionSource &lhs, const RestrictionSource &rhs)
    {
        return (lhs.start_node == rhs.start_node && lhs.via_node == rhs.via_node);
    }
};

struct RestrictionTarget
{
    NodeID target_node;
    bool is_only;

    explicit RestrictionTarget(NodeID target, bool only) : target_node(target), is_only(only)
    {
    }

    friend inline bool operator==(const RestrictionTarget &lhs, const RestrictionTarget &rhs)
    {
        return (lhs.target_node == rhs.target_node && lhs.is_only == rhs.is_only);
    }
};

namespace std
{
template <> struct hash<RestrictionSource>
{
    size_t operator()(const RestrictionSource &r_source) const
    {
        return hash_val(r_source.start_node, r_source.via_node);
    }
};

template <> struct hash<RestrictionTarget>
{
    size_t operator()(const RestrictionTarget &r_target) const
    {
        return hash_val(r_target.target_node, r_target.is_only);
    }
};
}

/**
    \brief Efficent look up if an edge is the start + via node of a TurnRestriction
    EdgeBasedEdgeFactory decides by it if edges are inserted or geometry is compressed
*/
class RestrictionMap
{
  public:
    RestrictionMap(const std::shared_ptr<NodeBasedDynamicGraph> &graph,
                   const std::vector<TurnRestriction> &input_restrictions_list);

    void FixupArrivingTurnRestriction(const NodeID u, const NodeID v, const NodeID w);
    void FixupStartingTurnRestriction(const NodeID u, const NodeID v, const NodeID w);
    NodeID CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const;
    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const;
    bool IsViaNode(const NodeID node) const;
    std::size_t size()
    {
        return m_count;
    }

  private:
    bool IsSourceNode(const NodeID node) const;
    using EmanatingRestrictionsVector = std::vector<RestrictionTarget>;
    using EdgeData = NodeBasedDynamicGraph::EdgeData;

    std::size_t m_count;
    std::shared_ptr<NodeBasedDynamicGraph> m_graph;
    //! index -> list of (target, isOnly)
    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    //! maps (start, via) -> bucket index
    std::unordered_map<RestrictionSource, unsigned> m_restriction_map;
    std::unordered_set<NodeID> m_restriction_start_nodes;
    std::unordered_set<NodeID> m_no_turn_via_node_set;
};

#endif //RESTRICTION_MAP_H_
