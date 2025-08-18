/*

Copyright (c) 2015, Project OSRM, Dennis Luxen, others
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

#ifndef CONTRACTOR_HPP
#define CONTRACTOR_HPP

#include "../data_structures/binary_heap.hpp"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/dynamic_graph.hpp"
#include "../data_structures/percent.hpp"
#include "../data_structures/query_edge.hpp"
#include "../data_structures/xor_fast_hash.hpp"
#include "../data_structures/xor_fast_hash_storage.hpp"
#include "../util/integer_range.hpp"
#include "../util/simple_logger.hpp"
#include "../util/timing_util.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <stxxl/vector>

#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <limits>
#include <vector>

class Contractor
{

  private:
    struct ContractorEdgeData
    {
        ContractorEdgeData()
            : distance(0), id(0), originalEdges(0), shortcut(0), forward(0), backward(0),
              is_original_via_node_ID(false)
        {
        }
        ContractorEdgeData(unsigned distance,
                           unsigned original_edges,
                           unsigned id,
                           bool shortcut,
                           bool forward,
                           bool backward)
            : distance(distance), id(id),
              originalEdges(std::min((unsigned)1 << 28, original_edges)), shortcut(shortcut),
              forward(forward), backward(backward), is_original_via_node_ID(false)
        {
        }
        unsigned distance;
        unsigned id;
        unsigned originalEdges : 28;
        bool shortcut : 1;
        bool forward : 1;
        bool backward : 1;
        bool is_original_via_node_ID : 1;
    } data;

    struct ContractorHeapData
    {
        short hop;
        bool target;
        ContractorHeapData() : hop(0), target(false) {}
        ContractorHeapData(short h, bool t) : hop(h), target(t) {}
    };

    using ContractorGraph = DynamicGraph<ContractorEdgeData>;
    //    using ContractorHeap = BinaryHeap<NodeID, NodeID, int, ContractorHeapData,
    //    ArrayStorage<NodeID, NodeID>
    //    >;
    using ContractorHeap =
        BinaryHeap<NodeID, NodeID, int, ContractorHeapData, XORFastHashStorage<NodeID, NodeID>>;
    using ContractorEdge = ContractorGraph::InputEdge;

    struct ContractorThreadData
    {
        ContractorHeap heap;
        std::vector<ContractorEdge> inserted_edges;
        std::vector<NodeID> neighbours;
        explicit ContractorThreadData(NodeID nodes) : heap(nodes) {}
    };

    struct NodePriorityData
    {
        int depth;
        NodePriorityData() : depth(0) {}
    };

    struct ContractionStats
    {
        int edges_deleted_count;
        int edges_added_count;
        int original_edges_deleted_count;
        int original_edges_added_count;
        ContractionStats()
            : edges_deleted_count(0), edges_added_count(0), original_edges_deleted_count(0),
              original_edges_added_count(0)
        {
        }
    };

    struct RemainingNodeData
    {
        RemainingNodeData() : id(0), is_independent(false) {}
        NodeID id : 31;
        bool is_independent : 1;
    };

    struct ThreadDataContainer
    {
        explicit ThreadDataContainer(int number_of_nodes) : number_of_nodes(number_of_nodes) {}

        inline ContractorThreadData *getThreadData()
        {
            bool exists = false;
            auto &ref = data.local(exists);
            if (!exists)
            {
                ref = std::make_shared<ContractorThreadData>(number_of_nodes);
            }

            return ref.get();
        }

        int number_of_nodes;
        using EnumerableThreadData =
            tbb::enumerable_thread_specific<std::shared_ptr<ContractorThreadData>>;
        EnumerableThreadData data;
    };

  public:
    template <class ContainerT> Contractor(int nodes, ContainerT &input_edge_list)
    {
        std::vector<ContractorEdge> edges;
        edges.reserve(input_edge_list.size() * 2);

        const auto dend = input_edge_list.dend();
        for (auto diter = input_edge_list.dbegin(); diter != dend; ++diter)
        {
            BOOST_ASSERT_MSG(static_cast<unsigned int>(std::max(diter->weight, 1)) > 0,
                             "edge distance < 1");
#ifndef NDEBUG
            if (static_cast<unsigned int>(std::max(diter->weight, 1)) > 24 * 60 * 60 * 10)
            {
                SimpleLogger().Write(logWARNING)
                    << "Edge weight large -> "
                    << static_cast<unsigned int>(std::max(diter->weight, 1));
            }
#endif
            edges.emplace_back(diter->source, diter->target,
                               static_cast<unsigned int>(std::max(diter->weight, 1)), 1,
                               diter->edge_id, false, diter->forward ? true : false,
                               diter->backward ? true : false);

            edges.emplace_back(diter->target, diter->source,
                               static_cast<unsigned int>(std::max(diter->weight, 1)), 1,
                               diter->edge_id, false, diter->backward ? true : false,
                               diter->forward ? true : false);
        }
        // clear input vector
        input_edge_list.clear();
        edges.shrink_to_fit();

        tbb::parallel_sort(edges.begin(), edges.end());
        NodeID edge = 0;
        for (NodeID i = 0; i < edges.size();)
        {
            const NodeID source = edges[i].source;
            const NodeID target = edges[i].target;
            const NodeID id = edges[i].data.id;
            // remove eigenloops
            if (source == target)
            {
                ++i;
                continue;
            }
            ContractorEdge forward_edge;
            ContractorEdge reverse_edge;
            forward_edge.source = reverse_edge.source = source;
            forward_edge.target = reverse_edge.target = target;
            forward_edge.data.forward = reverse_edge.data.backward = true;
            forward_edge.data.backward = reverse_edge.data.forward = false;
            forward_edge.data.shortcut = reverse_edge.data.shortcut = false;
            forward_edge.data.id = reverse_edge.data.id = id;
            forward_edge.data.originalEdges = reverse_edge.data.originalEdges = 1;
            forward_edge.data.distance = reverse_edge.data.distance =
                std::numeric_limits<int>::max();
            // remove parallel edges
            while (i < edges.size() && edges[i].source == source && edges[i].target == target)
            {
                if (edges[i].data.forward)
                {
                    forward_edge.data.distance =
                        std::min(edges[i].data.distance, forward_edge.data.distance);
                }
                if (edges[i].data.backward)
                {
                    reverse_edge.data.distance =
                        std::min(edges[i].data.distance, reverse_edge.data.distance);
                }
                ++i;
            }
            // merge edges (s,t) and (t,s) into bidirectional edge
            if (forward_edge.data.distance == reverse_edge.data.distance)
            {
                if ((int)forward_edge.data.distance != std::numeric_limits<int>::max())
                {
                    forward_edge.data.backward = true;
                    edges[edge++] = forward_edge;
                }
            }
            else
            { // insert seperate edges
                if (((int)forward_edge.data.distance) != std::numeric_limits<int>::max())
                {
                    edges[edge++] = forward_edge;
                }
                if ((int)reverse_edge.data.distance != std::numeric_limits<int>::max())
                {
                    edges[edge++] = reverse_edge;
                }
            }
        }
        std::cout << "merged " << edges.size() - edge << " edges out of " << edges.size()
                  << std::endl;
        edges.resize(edge);
        contractor_graph = std::make_shared<ContractorGraph>(nodes, edges);
        edges.clear();
        edges.shrink_to_fit();

        BOOST_ASSERT(0 == edges.capacity());
        //        unsigned maxdegree = 0;
        //        NodeID highestNode = 0;
        //
        //        for(unsigned i = 0; i < contractor_graph->GetNumberOfNodes(); ++i) {
        //            unsigned degree = contractor_graph->EndEdges(i) -
        //            contractor_graph->BeginEdges(i);
        //            if(degree > maxdegree) {
        //                maxdegree = degree;
        //                highestNode = i;
        //            }
        //        }
        //
        //        SimpleLogger().Write() << "edges at node with id " << highestNode << " has degree
        //        " << maxdegree;
        //        for(unsigned i = contractor_graph->BeginEdges(highestNode); i <
        //        contractor_graph->EndEdges(highestNode); ++i) {
        //            SimpleLogger().Write() << " ->(" << highestNode << "," <<
        //            contractor_graph->GetTarget(i)
        //            << "); via: " << contractor_graph->GetEdgeData(i).via;
        //        }

        std::cout << "contractor finished initalization" << std::endl;
    }

    ~Contractor() {}

    void Run()
    {
        // for the preperation we can use a big grain size, which is much faster (probably cache)
        constexpr size_t InitGrainSize = 100000;
        constexpr size_t PQGrainSize = 100000;
        // auto_partitioner will automatically increase the blocksize if we have
        // a lot of data. It is *important* for the last loop iterations
        // (which have a very small dataset) that it is devisible.
        constexpr size_t IndependentGrainSize = 1;
        constexpr size_t ContractGrainSize = 1;
        constexpr size_t NeighboursGrainSize = 1;
        constexpr size_t DeleteGrainSize = 1;

        const NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();
        Percent p(number_of_nodes);

        ThreadDataContainer thread_data_list(number_of_nodes);

        NodeID number_of_contracted_nodes = 0;
        std::vector<RemainingNodeData> remaining_nodes(number_of_nodes);
        std::vector<float> node_priorities(number_of_nodes);
        std::vector<NodePriorityData> node_data(number_of_nodes);

        // initialize priorities in parallel
        tbb::parallel_for(tbb::blocked_range<int>(0, number_of_nodes, InitGrainSize),
                          [&remaining_nodes](const tbb::blocked_range<int> &range)
                          {
                              for (int x = range.begin(); x != range.end(); ++x)
                              {
                                  remaining_nodes[x].id = x;
                              }
                          });

        std::cout << "initializing elimination PQ ..." << std::flush;
        tbb::parallel_for(tbb::blocked_range<int>(0, number_of_nodes, PQGrainSize),
                          [this, &node_priorities, &node_data, &thread_data_list](
                              const tbb::blocked_range<int> &range)
                          {
                              ContractorThreadData *data = thread_data_list.getThreadData();
                              for (int x = range.begin(); x != range.end(); ++x)
                              {
                                  node_priorities[x] =
                                      this->EvaluateNodePriority(data, &node_data[x], x);
                              }
                          });
        std::cout << "ok" << std::endl << "preprocessing " << number_of_nodes << " nodes ..."
                  << std::flush;

        bool flushed_contractor = false;
        while (number_of_nodes > 2 && number_of_contracted_nodes < number_of_nodes)
        {
            if (!flushed_contractor && (number_of_contracted_nodes > (number_of_nodes * 0.65)))
            {
                DeallocatingVector<ContractorEdge> new_edge_set; // this one is not explicitely
                                                                 // cleared since it goes out of
                                                                 // scope anywa
                std::cout << " [flush " << number_of_contracted_nodes << " nodes] " << std::flush;

                // Delete old heap data to free memory that we need for the coming operations
                thread_data_list.data.clear();

                // Create new priority array
                std::vector<float> new_node_priority(remaining_nodes.size());
                // this map gives the old IDs from the new ones, necessary to get a consistent graph
                // at the end of contraction
                orig_node_id_to_new_id_map.resize(remaining_nodes.size());
                // this map gives the new IDs from the old ones, necessary to remap targets from the
                // remaining graph
                std::vector<NodeID> new_node_id_from_orig_id_map(number_of_nodes, UINT_MAX);

                // build forward and backward renumbering map and remap ids in remaining_nodes and
                // Priorities.
                for (const auto new_node_id : osrm::irange<std::size_t>(0, remaining_nodes.size()))
                {
                    // create renumbering maps in both directions
                    orig_node_id_to_new_id_map[new_node_id] = remaining_nodes[new_node_id].id;
                    new_node_id_from_orig_id_map[remaining_nodes[new_node_id].id] = new_node_id;
                    new_node_priority[new_node_id] =
                        node_priorities[remaining_nodes[new_node_id].id];
                    remaining_nodes[new_node_id].id = new_node_id;
                }
                // walk over all nodes
                for (const auto i :
                     osrm::irange<std::size_t>(0, contractor_graph->GetNumberOfNodes()))
                {
                    const NodeID source = i;
                    for (auto current_edge : contractor_graph->GetAdjacentEdgeRange(source))
                    {
                        ContractorGraph::EdgeData &data =
                            contractor_graph->GetEdgeData(current_edge);
                        const NodeID target = contractor_graph->GetTarget(current_edge);
                        if (SPECIAL_NODEID == new_node_id_from_orig_id_map[i])
                        {
                            external_edge_list.push_back({source, target, data});
                        }
                        else
                        {
                            // node is not yet contracted.
                            // add (renumbered) outgoing edges to new DynamicGraph.
                            ContractorEdge new_edge = {new_node_id_from_orig_id_map[source],
                                                       new_node_id_from_orig_id_map[target],
                                                       data};

                            new_edge.data.is_original_via_node_ID = true;
                            BOOST_ASSERT_MSG(UINT_MAX != new_node_id_from_orig_id_map[source],
                                             "new source id not resolveable");
                            BOOST_ASSERT_MSG(UINT_MAX != new_node_id_from_orig_id_map[target],
                                             "new target id not resolveable");
                            new_edge_set.push_back(new_edge);
                        }
                    }
                }

                // Delete map from old NodeIDs to new ones.
                new_node_id_from_orig_id_map.clear();
                new_node_id_from_orig_id_map.shrink_to_fit();

                // Replace old priorities array by new one
                node_priorities.swap(new_node_priority);
                // Delete old node_priorities vector
                new_node_priority.clear();
                new_node_priority.shrink_to_fit();
                // old Graph is removed
                contractor_graph.reset();

                // create new graph
                std::sort(new_edge_set.begin(), new_edge_set.end());
                contractor_graph =
                    std::make_shared<ContractorGraph>(remaining_nodes.size(), new_edge_set);

                new_edge_set.clear();
                flushed_contractor = true;

                // INFO: MAKE SURE THIS IS THE LAST OPERATION OF THE FLUSH!
                // reinitialize heaps and ThreadData objects with appropriate size
                thread_data_list.number_of_nodes = contractor_graph->GetNumberOfNodes();
            }

            const int last = (int)remaining_nodes.size();
            tbb::parallel_for(tbb::blocked_range<int>(0, last, IndependentGrainSize),
                              [this, &node_priorities, &remaining_nodes, &thread_data_list](
                                  const tbb::blocked_range<int> &range)
                              {
                                  ContractorThreadData *data = thread_data_list.getThreadData();
                                  // determine independent node set
                                  for (int i = range.begin(); i != range.end(); ++i)
                                  {
                                      const NodeID node = remaining_nodes[i].id;
                                      remaining_nodes[i].is_independent =
                                          this->IsNodeIndependent(node_priorities, data, node);
                                  }
                              });

            const auto first = stable_partition(remaining_nodes.begin(), remaining_nodes.end(),
                                                [](RemainingNodeData node_data)
                                                {
                                                    return !node_data.is_independent;
                                                });
            const int first_independent_node = static_cast<int>(first - remaining_nodes.begin());

            // contract independent nodes
            tbb::parallel_for(
                tbb::blocked_range<int>(first_independent_node, last, ContractGrainSize),
                [this, &remaining_nodes, &thread_data_list](const tbb::blocked_range<int> &range)
                {
                    ContractorThreadData *data = thread_data_list.getThreadData();
                    for (int position = range.begin(); position != range.end(); ++position)
                    {
                        const NodeID x = remaining_nodes[position].id;
                        this->ContractNode<false>(data, x);
                    }
                });
            // make sure we really sort each block
            tbb::parallel_for(
                thread_data_list.data.range(),
                [&](const ThreadDataContainer::EnumerableThreadData::range_type &range)
                {
                    for (auto &data : range)
                        std::sort(data->inserted_edges.begin(), data->inserted_edges.end());
                });
            tbb::parallel_for(
                tbb::blocked_range<int>(first_independent_node, last, DeleteGrainSize),
                [this, &remaining_nodes, &thread_data_list](const tbb::blocked_range<int> &range)
                {
                    ContractorThreadData *data = thread_data_list.getThreadData();
                    for (int position = range.begin(); position != range.end(); ++position)
                    {
                        const NodeID x = remaining_nodes[position].id;
                        this->DeleteIncomingEdges(data, x);
                    }
                });

            // insert new edges
            for (auto &data : thread_data_list.data)
            {
                for (const ContractorEdge &edge : data->inserted_edges)
                {
                    const EdgeID current_edge_ID =
                        contractor_graph->FindEdge(edge.source, edge.target);
                    if (current_edge_ID < contractor_graph->EndEdges(edge.source))
                    {
                        ContractorGraph::EdgeData &current_data =
                            contractor_graph->GetEdgeData(current_edge_ID);
                        if (current_data.shortcut && edge.data.forward == current_data.forward &&
                            edge.data.backward == current_data.backward &&
                            edge.data.distance < current_data.distance)
                        {
                            // found a duplicate edge with smaller weight, update it.
                            current_data = edge.data;
                            continue;
                        }
                    }
                    contractor_graph->InsertEdge(edge.source, edge.target, edge.data);
                }
                data->inserted_edges.clear();
            }

            tbb::parallel_for(
                tbb::blocked_range<int>(first_independent_node, last, NeighboursGrainSize),
                [this, &remaining_nodes, &node_priorities, &node_data, &thread_data_list](
                    const tbb::blocked_range<int> &range)
                {
                    ContractorThreadData *data = thread_data_list.getThreadData();
                    for (int position = range.begin(); position != range.end(); ++position)
                    {
                        NodeID x = remaining_nodes[position].id;
                        this->UpdateNodeNeighbours(node_priorities, node_data, data, x);
                    }
                });

            // remove contracted nodes from the pool
            number_of_contracted_nodes += last - first_independent_node;
            remaining_nodes.resize(first_independent_node);
            remaining_nodes.shrink_to_fit();
            //            unsigned maxdegree = 0;
            //            unsigned avgdegree = 0;
            //            unsigned mindegree = UINT_MAX;
            //            unsigned quaddegree = 0;
            //
            //            for(unsigned i = 0; i < remaining_nodes.size(); ++i) {
            //                unsigned degree = contractor_graph->EndEdges(remaining_nodes[i].first)
            //                -
            //                contractor_graph->BeginEdges(remaining_nodes[i].first);
            //                if(degree > maxdegree)
            //                    maxdegree = degree;
            //                if(degree < mindegree)
            //                    mindegree = degree;
            //
            //                avgdegree += degree;
            //                quaddegree += (degree*degree);
            //            }
            //
            //            avgdegree /= std::max((unsigned)1,(unsigned)remaining_nodes.size() );
            //            quaddegree /= std::max((unsigned)1,(unsigned)remaining_nodes.size() );
            //
            //            SimpleLogger().Write() << "rest: " << remaining_nodes.size() << ", max: "
            //            << maxdegree << ", min: " << mindegree << ", avg: " << avgdegree << ",
            //            quad: " << quaddegree;

            p.printStatus(number_of_contracted_nodes);
        }

        thread_data_list.data.clear();
    }

    template <class Edge> inline void GetEdges(DeallocatingVector<Edge> &edges)
    {
        Percent p(contractor_graph->GetNumberOfNodes());
        SimpleLogger().Write() << "Getting edges of minimized graph";
        const NodeID number_of_nodes = contractor_graph->GetNumberOfNodes();
        if (contractor_graph->GetNumberOfNodes())
        {
            Edge new_edge;
            for (const auto node : osrm::irange(0u, number_of_nodes))
            {
                p.printStatus(node);
                for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
                {
                    const NodeID target = contractor_graph->GetTarget(edge);
                    const ContractorGraph::EdgeData &data = contractor_graph->GetEdgeData(edge);
                    if (!orig_node_id_to_new_id_map.empty())
                    {
                        new_edge.source = orig_node_id_to_new_id_map[node];
                        new_edge.target = orig_node_id_to_new_id_map[target];
                    }
                    else
                    {
                        new_edge.source = node;
                        new_edge.target = target;
                    }
                    BOOST_ASSERT_MSG(UINT_MAX != new_edge.source, "Source id invalid");
                    BOOST_ASSERT_MSG(UINT_MAX != new_edge.target, "Target id invalid");
                    new_edge.data.distance = data.distance;
                    new_edge.data.shortcut = data.shortcut;
                    if (!data.is_original_via_node_ID && !orig_node_id_to_new_id_map.empty())
                    {
                        new_edge.data.id = orig_node_id_to_new_id_map[data.id];
                    }
                    else
                    {
                        new_edge.data.id = data.id;
                    }
                    BOOST_ASSERT_MSG(new_edge.data.id != INT_MAX, // 2^31
                                     "edge id invalid");
                    new_edge.data.forward = data.forward;
                    new_edge.data.backward = data.backward;
                    edges.push_back(new_edge);
                }
            }
        }
        contractor_graph.reset();
        orig_node_id_to_new_id_map.clear();
        orig_node_id_to_new_id_map.shrink_to_fit();

        BOOST_ASSERT(0 == orig_node_id_to_new_id_map.capacity());

        edges.append(external_edge_list.begin(), external_edge_list.end());
        external_edge_list.clear();
    }

  private:
    inline void Dijkstra(const int max_distance,
                         const unsigned number_of_targets,
                         const int maxNodes,
                         ContractorThreadData *const data,
                         const NodeID middleNode)
    {

        ContractorHeap &heap = data->heap;

        int nodes = 0;
        unsigned number_of_targets_found = 0;
        while (!heap.Empty())
        {
            const NodeID node = heap.DeleteMin();
            const int distance = heap.GetKey(node);
            const short current_hop = heap.GetData(node).hop + 1;

            if (++nodes > maxNodes)
            {
                return;
            }
            if (distance > max_distance)
            {
                return;
            }

            // Destination settled?
            if (heap.GetData(node).target)
            {
                ++number_of_targets_found;
                if (number_of_targets_found >= number_of_targets)
                {
                    return;
                }
            }

            // iterate over all edges of node
            for (auto edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &data = contractor_graph->GetEdgeData(edge);
                if (!data.forward)
                {
                    continue;
                }
                const NodeID to = contractor_graph->GetTarget(edge);
                if (middleNode == to)
                {
                    continue;
                }
                const int to_distance = distance + data.distance;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!heap.WasInserted(to))
                {
                    heap.Insert(to, to_distance, ContractorHeapData(current_hop, false));
                }
                // Found a shorter Path -> Update distance
                else if (to_distance < heap.GetKey(to))
                {
                    heap.DecreaseKey(to, to_distance);
                    heap.GetData(to).hop = current_hop;
                }
            }
        }
    }

    inline float EvaluateNodePriority(ContractorThreadData *const data,
                                      NodePriorityData *const node_data,
                                      const NodeID node)
    {
        ContractionStats stats;

        // perform simulated contraction
        ContractNode<true>(data, node, &stats);

        // Result will contain the priority
        float result;
        if (0 == (stats.edges_deleted_count * stats.original_edges_deleted_count))
        {
            result = 1.f * node_data->depth;
        }
        else
        {
            result = 2.f * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
                     4.f * (((float)stats.original_edges_added_count) /
                            stats.original_edges_deleted_count) +
                     1.f * node_data->depth;
        }
        BOOST_ASSERT(result >= 0);
        return result;
    }

    template <bool RUNSIMULATION>
    inline bool
    ContractNode(ContractorThreadData *data, const NodeID node, ContractionStats *stats = nullptr)
    {
        ContractorHeap &heap = data->heap;
        int inserted_edges_size = data->inserted_edges.size();
        std::vector<ContractorEdge> &inserted_edges = data->inserted_edges;

        for (auto in_edge : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &in_data = contractor_graph->GetEdgeData(in_edge);
            const NodeID source = contractor_graph->GetTarget(in_edge);
            if (RUNSIMULATION)
            {
                BOOST_ASSERT(stats != nullptr);
                ++stats->edges_deleted_count;
                stats->original_edges_deleted_count += in_data.originalEdges;
            }
            if (!in_data.backward)
            {
                continue;
            }

            heap.Clear();
            heap.Insert(source, 0, ContractorHeapData());
            int max_distance = 0;
            unsigned number_of_targets = 0;

            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                const int path_distance = in_data.distance + out_data.distance;
                max_distance = std::max(max_distance, path_distance);
                if (!heap.WasInserted(target))
                {
                    heap.Insert(target, INT_MAX, ContractorHeapData(0, true));
                    ++number_of_targets;
                }
            }

            if (RUNSIMULATION)
            {
                Dijkstra(max_distance, number_of_targets, 1000, data, node);
            }
            else
            {
                Dijkstra(max_distance, number_of_targets, 2000, data, node);
            }
            for (auto out_edge : contractor_graph->GetAdjacentEdgeRange(node))
            {
                const ContractorEdgeData &out_data = contractor_graph->GetEdgeData(out_edge);
                if (!out_data.forward)
                {
                    continue;
                }
                const NodeID target = contractor_graph->GetTarget(out_edge);
                const int path_distance = in_data.distance + out_data.distance;
                const int distance = heap.GetKey(target);
                if (path_distance < distance)
                {
                    if (RUNSIMULATION)
                    {
                        BOOST_ASSERT(stats != nullptr);
                        stats->edges_added_count += 2;
                        stats->original_edges_added_count +=
                            2 * (out_data.originalEdges + in_data.originalEdges);
                    }
                    else
                    {
                        inserted_edges.emplace_back(source, target, path_distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node, true, true, false);

                        inserted_edges.emplace_back(target, source, path_distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node, true, false, true);
                    }
                }
            }
        }
        if (!RUNSIMULATION)
        {
            int iend = inserted_edges.size();
            for (int i = inserted_edges_size; i < iend; ++i)
            {
                bool found = false;
                for (int other = i + 1; other < iend; ++other)
                {
                    if (inserted_edges[other].source != inserted_edges[i].source)
                    {
                        continue;
                    }
                    if (inserted_edges[other].target != inserted_edges[i].target)
                    {
                        continue;
                    }
                    if (inserted_edges[other].data.distance != inserted_edges[i].data.distance)
                    {
                        continue;
                    }
                    if (inserted_edges[other].data.shortcut != inserted_edges[i].data.shortcut)
                    {
                        continue;
                    }
                    inserted_edges[other].data.forward |= inserted_edges[i].data.forward;
                    inserted_edges[other].data.backward |= inserted_edges[i].data.backward;
                    found = true;
                    break;
                }
                if (!found)
                {
                    inserted_edges[inserted_edges_size++] = inserted_edges[i];
                }
            }
            inserted_edges.resize(inserted_edges_size);
        }
        return true;
    }

    inline void DeleteIncomingEdges(ContractorThreadData *data, const NodeID node)
    {
        std::vector<NodeID> &neighbours = data->neighbours;
        neighbours.clear();

        // find all neighbours
        for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const NodeID u = contractor_graph->GetTarget(e);
            if (u != node)
            {
                neighbours.push_back(u);
            }
        }
        // eliminate duplicate entries ( forward + backward edges )
        std::sort(neighbours.begin(), neighbours.end());
        neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

        for (const auto i : osrm::irange<std::size_t>(0, neighbours.size()))
        {
            contractor_graph->DeleteEdgesTo(neighbours[i], node);
        }
    }

    inline bool UpdateNodeNeighbours(std::vector<float> &priorities,
                                     std::vector<NodePriorityData> &node_data,
                                     ContractorThreadData *const data,
                                     const NodeID node)
    {
        std::vector<NodeID> &neighbours = data->neighbours;
        neighbours.clear();

        // find all neighbours
        for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const NodeID u = contractor_graph->GetTarget(e);
            if (u == node)
            {
                continue;
            }
            neighbours.push_back(u);
            node_data[u].depth = (std::max)(node_data[node].depth + 1, node_data[u].depth);
        }
        // eliminate duplicate entries ( forward + backward edges )
        std::sort(neighbours.begin(), neighbours.end());
        neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

        // re-evaluate priorities of neighboring nodes
        for (const NodeID u : neighbours)
        {
            priorities[u] = EvaluateNodePriority(data, &(node_data)[u], u);
        }
        return true;
    }

    inline bool IsNodeIndependent(const std::vector<float> &priorities,
                                  ContractorThreadData *const data,
                                  NodeID node) const
    {
        const float priority = priorities[node];

        std::vector<NodeID> &neighbours = data->neighbours;
        neighbours.clear();

        for (auto e : contractor_graph->GetAdjacentEdgeRange(node))
        {
            const NodeID target = contractor_graph->GetTarget(e);
            if (node == target)
            {
                continue;
            }
            const float target_priority = priorities[target];
            BOOST_ASSERT(target_priority >= 0);
            // found a neighbour with lower priority?
            if (priority > target_priority)
            {
                return false;
            }
            // tie breaking
            if (std::abs(priority - target_priority) < std::numeric_limits<float>::epsilon() &&
                bias(node, target))
            {
                return false;
            }
            neighbours.push_back(target);
        }

        std::sort(neighbours.begin(), neighbours.end());
        neighbours.resize(std::unique(neighbours.begin(), neighbours.end()) - neighbours.begin());

        // examine all neighbours that are at most 2 hops away
        for (const NodeID u : neighbours)
        {
            for (auto e : contractor_graph->GetAdjacentEdgeRange(u))
            {
                const NodeID target = contractor_graph->GetTarget(e);
                if (node == target)
                {
                    continue;
                }
                const float target_priority = priorities[target];
                BOOST_ASSERT(target_priority >= 0);
                // found a neighbour with lower priority?
                if (priority > target_priority)
                {
                    return false;
                }
                // tie breaking
                if (std::abs(priority - target_priority) < std::numeric_limits<float>::epsilon() &&
                    bias(node, target))
                {
                    return false;
                }
            }
        }
        return true;
    }

    // This bias function takes up 22 assembly instructions in total on X86
    inline bool bias(const NodeID a, const NodeID b) const
    {
        const unsigned short hasha = fast_hash(a);
        const unsigned short hashb = fast_hash(b);

        // The compiler optimizes that to conditional register flags but without branching
        // statements!
        if (hasha != hashb)
        {
            return hasha < hashb;
        }
        return a < b;
    }

    std::shared_ptr<ContractorGraph> contractor_graph;
    std::vector<ContractorGraph::InputEdge> contracted_edge_list;
    stxxl::vector<QueryEdge> external_edge_list;
    std::vector<NodeID> orig_node_id_to_new_id_map;
    XORFastHash fast_hash;
};

#endif // CONTRACTOR_HPP
