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

#ifndef BFS_COMPONENTS_HPP_
#define BFS_COMPONENTS_HPP_

#include "../typedefs.h"
#include "../data_structures/restriction_map.hpp"

#include <queue>
#include <unordered_set>

// Explores the components of the given graph while respecting turn restrictions
// and barriers.
template <typename GraphT> class BFSComponentExplorer
{
  public:
    BFSComponentExplorer(const GraphT &dynamic_graph,
                         const RestrictionMap &restrictions,
                         const std::unordered_set<NodeID> &barrier_nodes)
        : m_graph(dynamic_graph), m_restriction_map(restrictions), m_barrier_nodes(barrier_nodes)
    {
        BOOST_ASSERT(m_graph.GetNumberOfNodes() > 0);
    }

    /*!
     * Returns the size of the component that the node belongs to.
     */
    unsigned int GetComponentSize(const NodeID node) const
    {
        BOOST_ASSERT(node < m_component_index_list.size());

        return m_component_index_size[m_component_index_list[node]];
    }

    unsigned int GetNumberOfComponents() { return m_component_index_size.size(); }

    /*!
     * Computes the component sizes.
     */
    void run()
    {
        std::queue<std::pair<NodeID, NodeID>> bfs_queue;
        unsigned current_component = 0;

        BOOST_ASSERT(m_component_index_list.empty());
        BOOST_ASSERT(m_component_index_size.empty());

        unsigned num_nodes = m_graph.GetNumberOfNodes();

        m_component_index_list.resize(num_nodes, std::numeric_limits<unsigned>::max());

        BOOST_ASSERT(num_nodes > 0);

        // put unexplorered node with parent pointer into queue
        for (NodeID node = 0; node < num_nodes; ++node)
        {
            if (std::numeric_limits<unsigned>::max() == m_component_index_list[node])
            {
                unsigned size = ExploreComponent(bfs_queue, node, current_component);

                // push size into vector
                m_component_index_size.emplace_back(size);
                ++current_component;
            }
        }
    }

  private:
    /*!
     * Explores the current component that starts at node using BFS.
     */
    unsigned ExploreComponent(std::queue<std::pair<NodeID, NodeID>> &bfs_queue,
                              NodeID node,
                              unsigned current_component)
    {
        /*
           Graphical representation of variables:

           u           v           w
           *---------->*---------->*
                            e2
        */

        bfs_queue.emplace(node, node);
        // mark node as read
        m_component_index_list[node] = current_component;

        unsigned current_component_size = 1;

        while (!bfs_queue.empty())
        {
            // fetch element from BFS queue
            std::pair<NodeID, NodeID> current_queue_item = bfs_queue.front();
            bfs_queue.pop();

            const NodeID v = current_queue_item.first;  // current node
            const NodeID u = current_queue_item.second; // parent
            // increment size counter of current component
            ++current_component_size;
            if (m_barrier_nodes.find(v) != m_barrier_nodes.end())
            {
                continue;
            }
            const NodeID to_node_of_only_restriction =
                m_restriction_map.CheckForEmanatingIsOnlyTurn(u, v);

            for (auto e2 : m_graph.GetAdjacentEdgeRange(v))
            {
                const NodeID w = m_graph.GetTarget(e2);

                if (to_node_of_only_restriction != std::numeric_limits<unsigned>::max() &&
                    w != to_node_of_only_restriction)
                {
                    // At an only_-restriction but not at the right turn
                    continue;
                }

                if (u != w)
                {
                    // only add an edge if turn is not a U-turn except
                    // when it is at the end of a dead-end street.
                    if (!m_restriction_map.CheckIfTurnIsRestricted(u, v, w))
                    {
                        // only add an edge if turn is not prohibited
                        if (std::numeric_limits<unsigned>::max() == m_component_index_list[w])
                        {
                            // insert next (node, parent) only if w has
                            // not yet been explored
                            // mark node as read
                            m_component_index_list[w] = current_component;
                            bfs_queue.emplace(w, v);
                        }
                    }
                }
            }
        }

        return current_component_size;
    }

    std::vector<unsigned> m_component_index_list;
    std::vector<NodeID> m_component_index_size;

    const GraphT &m_graph;
    const RestrictionMap &m_restriction_map;
    const std::unordered_set<NodeID> &m_barrier_nodes;
};

#endif // BFS_COMPONENTS_HPP_
