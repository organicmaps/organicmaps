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

#ifndef DYNAMICGRAPH_H
#define DYNAMICGRAPH_H

#include "DeallocatingVector.h"
#include "Range.h"

#include <boost/assert.hpp>

#include <cstdint>

#include <algorithm>
#include <limits>
#include <vector>
#include <atomic>

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

        InputEdge() : source(std::numeric_limits<NodeIterator>::max()), target(std::numeric_limits<NodeIterator>::max()) { }

        template<typename... Ts>
        InputEdge(NodeIterator source, NodeIterator target, Ts &&...data) : source(source), target(target), data(std::forward<Ts>(data)...) { }

        bool operator<(const InputEdge &right) const
        {
            if (source != right.source)
            {
                return source < right.source;
            }
            return target < right.target;
        }
    };

    // Constructs an empty graph with a given number of nodes.
    explicit DynamicGraph(NodeIterator nodes) : number_of_nodes(nodes), number_of_edges(0)
    {
        node_list.reserve(number_of_nodes);
        node_list.resize(number_of_nodes);

        edge_list.reserve(number_of_nodes * 1.1);
        edge_list.resize(number_of_nodes);
    }

    template <class ContainerT> DynamicGraph(const NodeIterator nodes, const ContainerT &graph)
    {
        number_of_nodes = nodes;
        number_of_edges = (EdgeIterator)graph.size();
        node_list.reserve(number_of_nodes + 1);
        node_list.resize(number_of_nodes + 1);
        EdgeIterator edge = 0;
        EdgeIterator position = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes))
        {
            EdgeIterator lastEdge = edge;
            while (edge < number_of_edges && graph[edge].source == node)
            {
                ++edge;
            }
            node_list[node].firstEdge = position;
            node_list[node].edges = edge - lastEdge;
            position += node_list[node].edges;
        }
        node_list.back().firstEdge = position;
        edge_list.reserve((std::size_t)edge_list.size() * 1.1);
        edge_list.resize(position);
        edge = 0;
        for (const auto node : osrm::irange(0u, number_of_nodes))
        {
            for (const auto i : osrm::irange(node_list[node].firstEdge,
                                              node_list[node].firstEdge + node_list[node].edges))
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

    unsigned GetOutDegree(const NodeIterator n) const { return node_list[n].edges; }

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
        return EdgeIterator(node_list[n].firstEdge);
    }

    EdgeIterator EndEdges(const NodeIterator n) const
    {
        return EdgeIterator(node_list[n].firstEdge + node_list[n].edges);
    }

    EdgeRange GetAdjacentEdgeRange(const NodeIterator node) const
    {
        return osrm::irange(BeginEdges(node), EndEdges(node));
    }

    NodeIterator InsertNode()
    {
        node_list.emplace_back(node_list.back());
        number_of_nodes += 1;

        return number_of_nodes;
    }

    // adds an edge. Invalidates edge iterators for the source node
    EdgeIterator InsertEdge(const NodeIterator from, const NodeIterator to, const EdgeDataT &data)
    {
        Node &node = node_list[from];
        EdgeIterator newFirstEdge = node.edges + node.firstEdge;
        if (newFirstEdge >= edge_list.size() || !isDummy(newFirstEdge))
        {
            if (node.firstEdge != 0 && isDummy(node.firstEdge - 1))
            {
                node.firstEdge--;
                edge_list[node.firstEdge] = edge_list[node.firstEdge + node.edges];
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
                    edge_list[newFirstEdge + i] = edge_list[node.firstEdge + i];
                    makeDummy(node.firstEdge + i);
                }
                for (const auto i : osrm::irange(node.edges + 1, newSize))
                {
                    makeDummy(newFirstEdge + i);
                }
                node.firstEdge = newFirstEdge;
            }
        }
        Edge &edge = edge_list[node.firstEdge + node.edges];
        edge.target = to;
        edge.data = data;
        ++number_of_edges;
        ++node.edges;
        return EdgeIterator(node.firstEdge + node.edges);
    }

    // removes an edge. Invalidates edge iterators for the source node
    void DeleteEdge(const NodeIterator source, const EdgeIterator e)
    {
        Node &node = node_list[source];
        --number_of_edges;
        --node.edges;
        BOOST_ASSERT(std::numeric_limits<unsigned>::max() != node.edges);
        const unsigned last = node.firstEdge + node.edges;
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
        node_list[source].edges -= deleted;

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
        return EndEdges(from);
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
        EdgeIterator firstEdge;
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

    std::vector<Node> node_list;
    DeallocatingVector<Edge> edge_list;
};

#endif // DYNAMICGRAPH_H
