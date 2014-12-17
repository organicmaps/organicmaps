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

#include "RestrictionMap.h"
#include "NodeBasedGraph.h"

bool RestrictionMap::IsViaNode(const NodeID node) const
{
    return m_no_turn_via_node_set.find(node) != m_no_turn_via_node_set.end();
}

RestrictionMap::RestrictionMap(const std::shared_ptr<NodeBasedDynamicGraph> &graph,
                               const std::vector<TurnRestriction> &restriction_list)
    : m_count(0), m_graph(graph)
{
    // decompose restriction consisting of a start, via and end node into a
    // a pair of starting edge and a list of all end nodes
    for (auto &restriction : restriction_list)
    {
        m_restriction_start_nodes.insert(restriction.fromNode);
        m_no_turn_via_node_set.insert(restriction.viaNode);

        RestrictionSource restriction_source = {restriction.fromNode, restriction.viaNode};

        unsigned index;
        auto restriction_iter = m_restriction_map.find(restriction_source);
        if (restriction_iter == m_restriction_map.end())
        {
            index = m_restriction_bucket_list.size();
            m_restriction_bucket_list.resize(index + 1);
            m_restriction_map.emplace(restriction_source, index);
        }
        else
        {
            index = restriction_iter->second;
            // Map already contains an is_only_*-restriction
            if (m_restriction_bucket_list.at(index).begin()->is_only)
            {
                continue;
            }
            else if (restriction.flags.isOnly)
            {
                // We are going to insert an is_only_*-restriction. There can be only one.
                m_count -= m_restriction_bucket_list.at(index).size();
                m_restriction_bucket_list.at(index).clear();
            }
        }
        ++m_count;
        m_restriction_bucket_list.at(index)
            .emplace_back(restriction.toNode, restriction.flags.isOnly);
    }
}

// Replace end v with w in each turn restriction containing u as via node
void RestrictionMap::FixupArrivingTurnRestriction(const NodeID node_u,
                                                  const NodeID node_v,
                                                  const NodeID node_w)
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);
    BOOST_ASSERT(node_w != SPECIAL_NODEID);

    if (!IsViaNode(node_u))
    {
        return;
    }

    // find all potential start edges. It is more efficent to get a (small) list
    // of potential start edges than iterating over all buckets
    std::vector<NodeID> predecessors;
    for (const EdgeID current_edge_id : m_graph->GetAdjacentEdgeRange(node_u))
    {
        const NodeID target = m_graph->GetTarget(current_edge_id);
        if (node_v != target)
        {
            predecessors.push_back(target);
        }
    }

    for (const NodeID node_x : predecessors)
    {
        const auto restriction_iterator = m_restriction_map.find({node_x, node_u});
        if (restriction_iterator == m_restriction_map.end())
        {
            continue;
        }

        const unsigned index = restriction_iterator->second;
        auto &bucket = m_restriction_bucket_list.at(index);
        for (RestrictionTarget &restriction_target : bucket)
        {
            if (node_v == restriction_target.target_node)
            {
                restriction_target.target_node = node_w;
            }
        }
    }
}

// Replaces start edge (v, w) with (u, w). Only start node changes.
void RestrictionMap::FixupStartingTurnRestriction(const NodeID node_u,
                                                  const NodeID node_v,
                                                  const NodeID node_w)
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);
    BOOST_ASSERT(node_w != SPECIAL_NODEID);

    if (!IsSourceNode(node_v))
    {
        return;
    }

    const auto restriction_iterator = m_restriction_map.find({node_v, node_w});
    if (restriction_iterator != m_restriction_map.end())
    {
        const unsigned index = restriction_iterator->second;
        // remove old restriction start (v,w)
        m_restriction_map.erase(restriction_iterator);
        m_restriction_start_nodes.emplace(node_u);
        // insert new restriction start (u,w) (pointing to index)
        RestrictionSource new_source = {node_u, node_w};
        m_restriction_map.emplace(new_source, index);
    }
}

// Check if edge (u, v) is the start of any turn restriction.
// If so returns id of first target node.
NodeID RestrictionMap::CheckForEmanatingIsOnlyTurn(const NodeID node_u, const NodeID node_v) const
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);

    if (!IsSourceNode(node_u))
    {
        return SPECIAL_NODEID;
    }

    const auto restriction_iter = m_restriction_map.find({node_u, node_v});
    if (restriction_iter != m_restriction_map.end())
    {
        const unsigned index = restriction_iter->second;
        const auto &bucket = m_restriction_bucket_list.at(index);
        for (const RestrictionTarget &restriction_target : bucket)
        {
            if (restriction_target.is_only)
            {
                return restriction_target.target_node;
            }
        }
    }
    return SPECIAL_NODEID;
}

// Checks if turn <u,v,w> is actually a turn restriction.
bool RestrictionMap::CheckIfTurnIsRestricted(const NodeID node_u,
                                             const NodeID node_v,
                                             const NodeID node_w) const
{
    BOOST_ASSERT(node_u != SPECIAL_NODEID);
    BOOST_ASSERT(node_v != SPECIAL_NODEID);
    BOOST_ASSERT(node_w != SPECIAL_NODEID);

    if (!IsSourceNode(node_u))
    {
        return false;
    }

    const auto restriction_iter = m_restriction_map.find({node_u, node_v});
    if (restriction_iter != m_restriction_map.end())
    {
        const unsigned index = restriction_iter->second;
        const auto &bucket = m_restriction_bucket_list.at(index);
        for (const RestrictionTarget &restriction_target : bucket)
        {
            if ((node_w == restriction_target.target_node) && // target found
                (!restriction_target.is_only)                 // and not an only_-restr.
                )
            {
                return true;
            }
        }
    }
    return false;
}

// check of node is the start of any restriction
bool RestrictionMap::IsSourceNode(const NodeID node) const
{
    if (m_restriction_start_nodes.find(node) == m_restriction_start_nodes.end())
    {
        return false;
    }
    return true;
}
