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

#ifndef STATIC_GRAPH_HPP
#define STATIC_GRAPH_HPP

#include "percent.hpp"
#include "shared_memory_vector_wrapper.hpp"
#include "../util/integer_range.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

template <typename EdgeDataT, bool UseSharedMemory = false> class StaticGraph
{
  public:
    using NodeIterator = NodeID;
    using EdgeIterator = NodeID;
    using EdgeData = EdgeDataT;
    using EdgeRange = osrm::range<EdgeIterator>;

    class InputEdge
    {
      public:
        NodeIterator source;
        NodeIterator target;
        EdgeDataT data;

        template <typename... Ts>
        InputEdge(NodeIterator source, NodeIterator target, Ts &&... data)
            : source(source), target(target), data(std::forward<Ts>(data)...)
        {
        }
        bool operator<(const InputEdge &right) const
        {
            if (source != right.source)
            {
                return source < right.source;
            }
            return target < right.target;
        }
    };

    struct NodeArrayEntry
    {
        // index of the first edge
        EdgeIterator first_edge;
    };

    struct EdgeArrayEntry
    {
        NodeID target;
        EdgeDataT data;
    };

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const
    {
        return osrm::irange(BeginEdges(node), EndEdges(node));
    }

    StaticGraph(const int nodes, std::vector<InputEdge> &graph)
    {
        number_of_nodes = nodes;
        number_of_edges = static_cast<EdgeIterator>(graph.size());
        node_array.resize(number_of_nodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes + 1))
        {
            EdgeIterator last_edge = edge;
            while (edge < number_of_edges && graph[edge].source == node)
            {
                ++edge;
            }
            node_array[node].first_edge = position; //=edge
            position += edge - last_edge;           // remove
        }
        edge_array.resize(position); //(edge)
        edge = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes))
        {
            EdgeIterator e = node_array[node + 1].first_edge;
            for (const auto i : osrm::irange(node_array[node].first_edge, e))
            {
                edge_array[i].target = graph[edge].target;
                edge_array[i].data = graph[edge].data;
                edge++;
            }
        }
    }

    StaticGraph(typename ShM<NodeArrayEntry, UseSharedMemory>::vector &nodes,
                typename ShM<EdgeArrayEntry, UseSharedMemory>::vector &edges)
    {
        number_of_nodes = static_cast<decltype(number_of_nodes)>(nodes.size() - 1);
        number_of_edges = static_cast<decltype(number_of_edges)>(edges.size());

        node_array.swap(nodes);
        edge_array.swap(edges);
    }

    unsigned GetNumberOfNodes() const { return number_of_nodes; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }

    unsigned GetOutDegree(const NodeIterator n) const { return EndEdges(n) - BeginEdges(n); }

    inline NodeIterator GetTarget(const EdgeIterator e) const
    {
        return NodeIterator(edge_array[e].target);
    }

    EdgeDataT &GetEdgeData(const EdgeIterator e) { return edge_array[e].data; }

    const EdgeDataT &GetEdgeData(const EdgeIterator e) const { return edge_array[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n).first_edge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array.at(n + 1).first_edge);
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        for (const auto i : osrm::irange(BeginEdges(from), EndEdges(from)))
        {
            if (to == edge_array[i].target)
            {
                return i;
            }
        }
        return SPECIAL_EDGEID;
    }

    // searches for a specific edge
    EdgeIterator FindSmallestEdge(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator smallest_edge = SPECIAL_EDGEID;
        EdgeWeight smallest_weight = INVALID_EDGE_WEIGHT;
        for (auto edge : GetAdjacentEdgeRange(from))
        {
            const NodeID target = GetTarget(edge);
            const EdgeWeight weight = GetEdgeData(edge).distance;
            if (target == to && weight < smallest_weight)
            {
                smallest_edge = edge;
                smallest_weight = weight;
            }
        }
        return smallest_edge;
    }

    EdgeIterator FindEdgeInEitherDirection(const NodeIterator from, const NodeIterator to) const
    {
        EdgeIterator tmp = FindEdge(from, to);
        return (SPECIAL_NODEID != tmp ? tmp : FindEdge(to, from));
    }

    EdgeIterator
    FindEdgeIndicateIfReverse(const NodeIterator from, const NodeIterator to, bool &result) const
    {
        EdgeIterator current_iterator = FindEdge(from, to);
        if (SPECIAL_NODEID == current_iterator)
        {
            current_iterator = FindEdge(to, from);
            if (SPECIAL_NODEID != current_iterator)
            {
                result = true;
            }
        }
        return current_iterator;
    }

  private:
    NodeIterator number_of_nodes;
    EdgeIterator number_of_edges;

    typename ShM<NodeArrayEntry, UseSharedMemory>::vector node_array;
    typename ShM<EdgeArrayEntry, UseSharedMemory>::vector edge_array;
};

#endif // STATIC_GRAPH_HPP
