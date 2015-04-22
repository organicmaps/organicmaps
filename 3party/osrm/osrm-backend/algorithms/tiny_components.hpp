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

#ifndef TINY_COMPONENTS_HPP
#define TINY_COMPONENTS_HPP

#include "../typedefs.h"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/import_edge.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/percent.hpp"
#include "../data_structures/restriction.hpp"
#include "../data_structures/restriction_map.hpp"
#include "../data_structures/turn_instructions.hpp"

#include "../util/integer_range.hpp"
#include "../util/simple_logger.hpp"
#include "../util/std_hash.hpp"
#include "../util/timing_util.hpp"

#include <osrm/coordinate.hpp>

#include <boost/assert.hpp>

#include <tbb/parallel_sort.h>

#include <cstdint>

#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename GraphT> class TarjanSCC
{
    struct TarjanStackFrame
    {
        explicit TarjanStackFrame(NodeID v, NodeID parent) : v(v), parent(parent) {}
        NodeID v;
        NodeID parent;
    };

    struct TarjanNode
    {
        TarjanNode() : index(SPECIAL_NODEID), low_link(SPECIAL_NODEID), on_stack(false) {}
        unsigned index;
        unsigned low_link;
        bool on_stack;
    };

    std::vector<unsigned> components_index;
    std::vector<NodeID> component_size_vector;
    std::shared_ptr<GraphT> m_node_based_graph;
    std::unordered_set<NodeID> barrier_node_set;
    RestrictionMap m_restriction_map;
    std::size_t size_one_counter;

  public:
    template <class ContainerT>
    TarjanSCC(std::shared_ptr<GraphT> graph,
              const RestrictionMap &restrictions,
              const ContainerT &barrier_node_list)
        : components_index(graph->GetNumberOfNodes(), SPECIAL_NODEID), m_node_based_graph(graph),
          m_restriction_map(restrictions), size_one_counter(0)
    {
        barrier_node_set.insert(std::begin(barrier_node_list), std::end(barrier_node_list));
        BOOST_ASSERT(m_node_based_graph->GetNumberOfNodes() > 0);
    }

    void run()
    {
        TIMER_START(SCC_RUN);
        const NodeID max_node_id = m_node_based_graph->GetNumberOfNodes();

        // The following is a hack to distinguish between stuff that happens
        // before the recursive call and stuff that happens after
        std::stack<TarjanStackFrame> recursion_stack;
        // true = stuff before, false = stuff after call
        std::stack<NodeID> tarjan_stack;
        std::vector<TarjanNode> tarjan_node_list(max_node_id);
        unsigned component_index = 0, size_of_current_component = 0;
        unsigned index = 0;
        std::vector<bool> processing_node_before_recursion(max_node_id, true);
        for (const NodeID node : osrm::irange(0u, max_node_id))
        {
            if (SPECIAL_NODEID == components_index[node])
            {
                recursion_stack.emplace(TarjanStackFrame(node, node));
            }

            while (!recursion_stack.empty())
            {
                TarjanStackFrame currentFrame = recursion_stack.top();
                const NodeID u = currentFrame.parent;
                const NodeID v = currentFrame.v;
                recursion_stack.pop();

                const bool before_recursion = processing_node_before_recursion[v];

                if (before_recursion && tarjan_node_list[v].index != UINT_MAX)
                {
                    continue;
                }

                if (before_recursion)
                {
                    // Mark frame to handle tail of recursion
                    recursion_stack.emplace(currentFrame);
                    processing_node_before_recursion[v] = false;

                    // Mark essential information for SCC
                    tarjan_node_list[v].index = index;
                    tarjan_node_list[v].low_link = index;
                    tarjan_stack.push(v);
                    tarjan_node_list[v].on_stack = true;
                    ++index;

                    const NodeID to_node_of_only_restriction =
                        m_restriction_map.CheckForEmanatingIsOnlyTurn(u, v);

                    for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(v))
                    {
                        const auto vprime = m_node_based_graph->GetTarget(current_edge);

                        // Traverse outgoing edges
                        if (barrier_node_set.find(v) != barrier_node_set.end() && u != vprime)
                        {
                            // continue;
                        }

                        if (to_node_of_only_restriction != std::numeric_limits<unsigned>::max() &&
                            vprime == to_node_of_only_restriction)
                        {
                            // At an only_-restriction but not at the right turn
                            // continue;
                        }

                        if (m_restriction_map.CheckIfTurnIsRestricted(u, v, vprime))
                        {
                            // continue;
                        }

                        if (SPECIAL_NODEID == tarjan_node_list[vprime].index)
                        {
                            recursion_stack.emplace(TarjanStackFrame(vprime, v));
                        }
                        else
                        {
                            if (tarjan_node_list[vprime].on_stack &&
                                tarjan_node_list[vprime].index < tarjan_node_list[v].low_link)
                            {
                                tarjan_node_list[v].low_link = tarjan_node_list[vprime].index;
                            }
                        }
                    }
                }
                else
                {
                    processing_node_before_recursion[v] = true;
                    tarjan_node_list[currentFrame.parent].low_link =
                        std::min(tarjan_node_list[currentFrame.parent].low_link,
                                 tarjan_node_list[v].low_link);
                    // after recursion, lets do cycle checking
                    // Check if we found a cycle. This is the bottom part of the recursion
                    if (tarjan_node_list[v].low_link == tarjan_node_list[v].index)
                    {
                        NodeID vprime;
                        do
                        {
                            vprime = tarjan_stack.top();
                            tarjan_stack.pop();
                            tarjan_node_list[vprime].on_stack = false;
                            components_index[vprime] = component_index;
                            ++size_of_current_component;
                        } while (v != vprime);

                        component_size_vector.emplace_back(size_of_current_component);

                        if (size_of_current_component > 1000)
                        {
                            SimpleLogger().Write() << "large component [" << component_index
                                                   << "]=" << size_of_current_component;
                        }

                        ++component_index;
                        size_of_current_component = 0;
                    }
                }
            }
        }

        TIMER_STOP(SCC_RUN);
        SimpleLogger().Write() << "SCC run took: " << TIMER_MSEC(SCC_RUN) / 1000. << "s";

        size_one_counter = std::count_if(component_size_vector.begin(), component_size_vector.end(),
                                         [](unsigned value)
                                         {
                                             return 1 == value;
                                         });
    }

    std::size_t get_number_of_components() const { return component_size_vector.size(); }

    std::size_t get_size_one_count() const { return size_one_counter; }

    unsigned get_component_size(const NodeID node) const
    {
        return component_size_vector[components_index[node]];
    }

    unsigned get_component_id(const NodeID node) const { return components_index[node]; }
};

#endif /* TINY_COMPONENTS_HPP */
