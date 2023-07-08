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

#ifndef DYNAMICGRAPH_HPP
#define DYNAMICGRAPH_HPP

#include "deallocating_vector.hpp"
#include "../util/integer_range.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <cstdint>

#include <algorithm>
#include <atomic>
#include <limits>
#include <tuple>
#include <vector>

template <typename EdgeDataT> class DynamicGraph
{
  public:
    using EdgeData = EdgeDataT;
    using NodeIterator = unsigned;
    using EdgeIterator = unsigned;
    using EdgeRange = osrm::range<EdgeIterator>;

    class InputEdge
    {
      public:
        NodeIterator source;
        NodeIterator target;
        EdgeDataT data;

        InputEdge()
            : source(std::numeric_limits<NodeIterator>::max()),
              target(std::numeric_limits<NodeIterator>::max())
        {
        }

        template <typename... Ts>
        InputEdge(NodeIterator source, NodeIterator target, Ts &&... data)
            : source(source), target(target), data(std::forward<Ts>(data)...)
        {
        }

        bool operator<(const InputEdge &rhs) const
        {
            return std::tie(source, target) < std::tie(rhs.source, rhs.target);
        }
    };

    // Constructs an empty graph with a given number of nodes.
    explicit DynamicGraph(NodeIterator nodes) : number_of_nodes(nodes), number_of_edges(0)
    {
        node_array.reserve(number_of_nodes);
        node_array.resize(number_of_nodes);

        edge_list.reserve(number_of_nodes * 1.1);
        edge_list.resize(number_of_nodes);
    }

    template <class ContainerT> DynamicGraph(const NodeIterator nodes, const ContainerT &graph)
    {
        number_of_nodes = nodes;
        number_of_edges = static_cast<EdgeIterator>(graph.size());
        // node_array.reserve(number_of_nodes + 1);
        node_array.resize(number_of_nodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes))
        {
            EdgeIterator last_edge = edge;
            while (edge < number_of_edges && graph[edge].source == node)
            {
                ++edge;
            }
            node_array[node].first_edge = position;
            node_array[node].edges = edge - last_edge;
            position += node_array[node].edges;
        }
        node_array.back().first_edge = position;
        edge_list.reserve(static_cast<std::size_t>(edge_list.size() * 1.1));
        edge_list.resize(position);
        edge = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes))
        {
            for (const auto i : osrm::irange(node_array[node].first_edge,
                                             node_array[node].first_edge + node_array[node].edges))
            {
                edge_list[i].target = graph[edge].target;
                edge_list[i].data = graph[edge].data;
                ++edge;
            }
        }
    }

    ~DynamicGraph() {}

    unsigned GetNumberOfNodes() const { return number_of_nodes; }

    unsigned GetNumberOfEdges() const { return number_of_edges; }

    unsigned GetOutDegree(const NodeIterator n) const { return node_array[n].edges; }

    unsigned GetDirectedOutDegree(const NodeIterator n) const
    {
        unsigned degree = 0;
        for (const auto edge : osrm::irange(BeginEdges(n), EndEdges(n)))
        {
            if (GetEdgeData(edge).forward)
            {
                ++degree;
            }
        }
        return degree;
    }

    NodeIterator GetTarget(const EdgeIterator e) const { return NodeIterator(edge_list[e].target); }

    void SetTarget(const EdgeIterator e, const NodeIterator n) { edge_list[e].target = n; }

    EdgeDataT &GetEdgeData(const EdgeIterator e) { return edge_list[e].data; }

    const EdgeDataT &GetEdgeData(const EdgeIterator e) const { return edge_list[e].data; }

    EdgeIterator BeginEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array[n].first_edge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_array[n].first_edge + node_array[n].edges);
    }

    EdgeRange GetAdjacentEdgeRange(const NodeIterator node) const
    {
        return osrm::irange(BeginEdges(node), EndEdges(node));
    }

    NodeIterator InsertNode()
    {
        node_array.emplace_back(node_array.back());
        number_of_nodes += 1;

        return number_of_nodes;
    }

    // adds an edge. Invalidates edge iterators for the source node
    EdgeIterator InsertEdge(const NodeIterator from, const NodeIterator to, const EdgeDataT &data)
    {
        Node &node = node_array[from];
        EdgeIterator newFirstEdge = node.edges + node.first_edge;
        if (newFirstEdge >= edge_list.size() || !isDummy(newFirstEdge))
        {
            if (node.first_edge != 0 && isDummy(node.first_edge - 1))
            {
                node.first_edge--;
                edge_list[node.first_edge] = edge_list[node.first_edge + node.edges];
            }
            else
            {
                EdgeIterator newFirstEdge = (EdgeIterator)edge_list.size();
                unsigned newSize = node.edges * 1.1 + 2;
                EdgeIterator requiredCapacity = newSize + edge_list.size();
                EdgeIterator oldCapacity = edge_list.capacity();
                if (requiredCapacity >= oldCapacity)
                {
                    edge_list.reserve(requiredCapacity * 1.1);
                }
                edge_list.resize(edge_list.size() + newSize);
                for (const auto i : osrm::irange(0u, node.edges))
                {
                    edge_list[newFirstEdge + i] = edge_list[node.first_edge + i];
                    makeDummy(node.first_edge + i);
                }
                for (const auto i : osrm::irange(node.edges + 1, newSize))
                {
                    makeDummy(newFirstEdge + i);
                }
                node.first_edge = newFirstEdge;
            }
        }
        Edge &edge = edge_list[node.first_edge + node.edges];
        edge.target = to;
        edge.data = data;
        ++number_of_edges;
        ++node.edges;
        return EdgeIterator(node.first_edge + node.edges);
    }

    // removes an edge. Invalidates edge iterators for the source node
    void DeleteEdge(const NodeIterator source, const EdgeIterator e)
    {
        Node &node = node_array[source];
        --number_of_edges;
        --node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != node.edges);
        const unsigned last = node.first_edge + node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != last);
        // swap with last edge
        edge_list[e] = edge_list[last];
        makeDummy(last);
    }

    // removes all edges (source,target)
    int32_t DeleteEdgesTo(const NodeIterator source, const NodeIterator target)
    {
        int32_t deleted = 0;
        for (EdgeIterator i = BeginEdges(source), iend = EndEdges(source); i < iend - deleted; ++i)
        {
            if (edge_list[i].target == target)
            {
                do
                {
                    deleted++;
                    edge_list[i] = edge_list[iend - deleted];
                    makeDummy(iend - deleted);
                } while (i < iend - deleted && edge_list[i].target == target);
            }
        }

        number_of_edges -= deleted;
        node_array[source].edges -= deleted;

        return deleted;
    }

    // searches for a specific edge
    EdgeIterator FindEdge(const NodeIterator from, const NodeIterator to) const
    {
        for (const auto i : osrm::irange(BeginEdges(from), EndEdges(from)))
        {
            if (to == edge_list[i].target)
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

  protected:
    bool isDummy(const EdgeIterator edge) const
    {
        return edge_list[edge].target == (std::numeric_limits<NodeIterator>::max)();
    }

    void makeDummy(const EdgeIterator edge)
    {
        edge_list[edge].target = (std::numeric_limits<NodeIterator>::max)();
    }

    struct Node
    {
        // index of the first edge
        EdgeIterator first_edge;
        // amount of edges
        unsigned edges;
    };

    struct Edge
    {
        NodeIterator target;
        EdgeDataT data;
    };

    NodeIterator number_of_nodes;
    std::atomic_uint number_of_edges;

    std::vector<Node> node_array;
    DeallocatingVector<Edge> edge_list;
};

#endif // DYNAMICGRAPH_HPP
