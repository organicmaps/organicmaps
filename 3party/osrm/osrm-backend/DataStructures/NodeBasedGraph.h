#ifndef NODE_BASED_GRAPH_H_
#define NODE_BASED_GRAPH_H_

#include "DynamicGraph.h"
#include "ImportEdge.h"
#include "../Util/simple_logger.hpp"

#include <tbb/parallel_sort.h>

#include <memory>

struct NodeBasedEdgeData
{
    NodeBasedEdgeData()
        : distance(INVALID_EDGE_WEIGHT), edgeBasedNodeID(SPECIAL_NODEID),
          nameID(std::numeric_limits<unsigned>::max()),
          isAccessRestricted(false), shortcut(false), forward(false), backward(false),
          roundabout(false), ignore_in_grid(false), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    int distance;
    unsigned edgeBasedNodeID;
    unsigned nameID;
    bool isAccessRestricted : 1;
    bool shortcut : 1;
    bool forward : 1;
    bool backward : 1;
    bool roundabout : 1;
    bool ignore_in_grid : 1;
    TravelMode travel_mode : 4;

    void SwapDirectionFlags()
    {
        bool temp_flag = forward;
        forward = backward;
        backward = temp_flag;
    }

    bool IsEqualTo(const NodeBasedEdgeData &other) const
    {
        return (forward == other.forward) && (backward == other.backward) &&
               (nameID == other.nameID) && (ignore_in_grid == other.ignore_in_grid) &&
               (travel_mode == other.travel_mode);
    }
};

struct SimpleEdgeData
{
    SimpleEdgeData() : capacity(0) {}
    EdgeWeight capacity;
};

using NodeBasedDynamicGraph = DynamicGraph<NodeBasedEdgeData>;
using SimpleNodeBasedDynamicGraph = DynamicGraph<SimpleEdgeData>;

// Factory method to create NodeBasedDynamicGraph from ImportEdges
inline std::shared_ptr<NodeBasedDynamicGraph>
NodeBasedDynamicGraphFromImportEdges(int number_of_nodes, std::vector<ImportEdge> &input_edge_list)
{
    static_assert(sizeof(NodeBasedEdgeData) == 16, "changing node based edge data size changes memory consumption");

    DeallocatingVector<NodeBasedDynamicGraph::InputEdge> edges_list;
    NodeBasedDynamicGraph::InputEdge edge;
    for (const ImportEdge &import_edge : input_edge_list)
    {
        if (import_edge.forward)
        {
            edge.source = import_edge.source;
            edge.target = import_edge.target;
            edge.data.forward = import_edge.forward;
            edge.data.backward = import_edge.backward;
        }
        else
        {
            edge.source = import_edge.target;
            edge.target = import_edge.source;
            edge.data.backward = import_edge.forward;
            edge.data.forward = import_edge.backward;
        }

        if (edge.source == edge.target)
        {
            continue;
        }

        edge.data.distance = (std::max)((int)import_edge.weight, 1);
        BOOST_ASSERT(edge.data.distance > 0);
        edge.data.shortcut = false;
        edge.data.roundabout = import_edge.roundabout;
        edge.data.ignore_in_grid = import_edge.in_tiny_cc;
        edge.data.nameID = import_edge.name_id;
        edge.data.isAccessRestricted = import_edge.access_restricted;
        edge.data.travel_mode = import_edge.travel_mode;

        edges_list.push_back(edge);

        if (!import_edge.is_split)
        {
            using std::swap; // enable ADL
            swap(edge.source, edge.target);
            edge.data.SwapDirectionFlags();
            edges_list.push_back(edge);
        }
    }

    // remove duplicate edges
    tbb::parallel_sort(edges_list.begin(), edges_list.end());
    NodeID edge_count = 0;
    for (NodeID i = 0; i < edges_list.size(); )
    {
        const NodeID source = edges_list[i].source;
        const NodeID target = edges_list[i].target;
        // remove eigenloops
        if (source == target)
        {
            i++;
            continue;
        }
        NodeBasedDynamicGraph::InputEdge forward_edge;
        NodeBasedDynamicGraph::InputEdge reverse_edge;
        forward_edge = reverse_edge = edges_list[i];
        forward_edge.data.forward = reverse_edge.data.backward = true;
        forward_edge.data.backward = reverse_edge.data.forward = false;
        forward_edge.data.shortcut = reverse_edge.data.shortcut = false;
        forward_edge.data.distance = reverse_edge.data.distance =
            std::numeric_limits<int>::max();
        // remove parallel edges
        while (i < edges_list.size() && edges_list[i].source == source && edges_list[i].target == target)
        {
            if (edges_list[i].data.forward)
            {
                forward_edge.data.distance =
                    std::min(edges_list[i].data.distance, forward_edge.data.distance);
            }
            if (edges_list[i].data.backward)
            {
                reverse_edge.data.distance =
                    std::min(edges_list[i].data.distance, reverse_edge.data.distance);
            }
            ++i;
        }
        // merge edges (s,t) and (t,s) into bidirectional edge
        if (forward_edge.data.distance == reverse_edge.data.distance)
        {
            if ((int)forward_edge.data.distance != std::numeric_limits<int>::max())
            {
                forward_edge.data.backward = true;
                edges_list[edge_count++] = forward_edge;
            }
        }
        else
        { // insert seperate edges
            if (((int)forward_edge.data.distance) != std::numeric_limits<int>::max())
            {
                edges_list[edge_count++] = forward_edge;
            }
            if ((int)reverse_edge.data.distance != std::numeric_limits<int>::max())
            {
                edges_list[edge_count++] = reverse_edge;
            }
        }
    }
    edges_list.resize(edge_count);
    SimpleLogger().Write() << "merged " << edges_list.size() - edge_count << " edges out of " << edges_list.size();

    auto graph = std::make_shared<NodeBasedDynamicGraph>(static_cast<NodeBasedDynamicGraph::NodeIterator>(number_of_nodes), edges_list);
    return graph;
}

template<class SimpleEdgeT>
inline std::shared_ptr<SimpleNodeBasedDynamicGraph>
SimpleNodeBasedDynamicGraphFromEdges(int number_of_nodes, std::vector<SimpleEdgeT> &input_edge_list)
{
    static_assert(sizeof(NodeBasedEdgeData) == 16, "changing node based edge data size changes memory consumption");
    tbb::parallel_sort(input_edge_list.begin(), input_edge_list.end());

    DeallocatingVector<SimpleNodeBasedDynamicGraph::InputEdge> edges_list;
    SimpleNodeBasedDynamicGraph::InputEdge edge;
    edge.data.capacity = 1;
    for (const SimpleEdgeT &import_edge : input_edge_list)
    {
        if (import_edge.source == import_edge.target)
        {
            continue;
        }
        edge.source = import_edge.source;
        edge.target = import_edge.target;
        edges_list.push_back(edge);
        std::swap(edge.source, edge.target);
        edges_list.push_back(edge);
    }

     // remove duplicate edges
    tbb::parallel_sort(edges_list.begin(), edges_list.end());
    NodeID edge_count = 0;
    for (NodeID i = 0; i < edges_list.size(); )
    {
        const NodeID source = edges_list[i].source;
        const NodeID target = edges_list[i].target;
        // remove eigenloops
        if (source == target)
        {
            i++;
            continue;
        }
        SimpleNodeBasedDynamicGraph::InputEdge forward_edge;
        SimpleNodeBasedDynamicGraph::InputEdge reverse_edge;
        forward_edge = reverse_edge = edges_list[i];
        forward_edge.data.capacity = reverse_edge.data.capacity = INVALID_EDGE_WEIGHT;
        // remove parallel edges
        while (i < edges_list.size() && edges_list[i].source == source && edges_list[i].target == target)
        {
            forward_edge.data.capacity = std::min(edges_list[i].data.capacity, forward_edge.data.capacity);
            reverse_edge.data.capacity = std::min(edges_list[i].data.capacity, reverse_edge.data.capacity);
            ++i;
        }
        // merge edges (s,t) and (t,s) into bidirectional edge
        if (forward_edge.data.capacity == reverse_edge.data.capacity)
        {
            if ((int)forward_edge.data.capacity != INVALID_EDGE_WEIGHT)
            {
                edges_list[edge_count++] = forward_edge;
            }
        }
        else
        { // insert seperate edges
            if (((int)forward_edge.data.capacity) != INVALID_EDGE_WEIGHT)
            {
                edges_list[edge_count++] = forward_edge;
            }
            if ((int)reverse_edge.data.capacity != INVALID_EDGE_WEIGHT)
            {
                edges_list[edge_count++] = reverse_edge;
            }
        }
    }
    SimpleLogger().Write() << "merged " << edges_list.size() - edge_count << " edges out of " << edges_list.size();

    auto graph = std::make_shared<SimpleNodeBasedDynamicGraph>(number_of_nodes, edges_list);
    return graph;
}

#endif // NODE_BASED_GRAPH_H_
