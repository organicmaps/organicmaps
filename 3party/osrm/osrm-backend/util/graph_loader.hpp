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

#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include "fingerprint.hpp"
#include "osrm_exception.hpp"
#include "simple_logger.hpp"
#include "../data_structures/external_memory_node.hpp"
#include "../data_structures/import_edge.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/restriction.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <cmath>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <vector>

template <typename EdgeT>
NodeID read_undirected_osrm_stream(std::istream &input_stream,
                                   std::vector<EdgeT> &edge_list,
                                   std::vector<FixedPointCoordinate> &coordinate_list)
{
    const FingerPrint fingerprint_orig;
    FingerPrint fingerprint_loaded;
    input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));

    if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".osrm was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    std::unordered_map<NodeID, NodeID> ext_to_int_id_map;

    NodeID n;
    input_stream.read(reinterpret_cast<char *>(&n), sizeof(NodeID));
    SimpleLogger().Write() << "Importing n = " << n << " nodes ";

    ExternalMemoryNode current_node;
    for (NodeID i = 0; i < n; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&current_node), sizeof(ExternalMemoryNode));
        coordinate_list.emplace_back(current_node.lat, current_node.lon);
        ext_to_int_id_map.emplace(current_node.node_id, i);
        // if (current_node.barrier)
        // {
        //     barrier_node_list.emplace_back(i);
        // }
        // if (current_node.traffic_lights)
        // {
        //     traffic_light_node_list.emplace_back(i);
        // }
    }

    // tighten vector sizes
    // barrier_node_list.shrink_to_fit();
    // traffic_light_node_list.shrink_to_fit();

    // renumber nodes in turn restrictions
    // for (TurnRestriction &current_restriction : restriction_list)
    // {
    //     auto internal_id_iter = ext_to_int_id_map.find(current_restriction.from.node);
    //     if (internal_id_iter == ext_to_int_id_map.end())
    //     {
    //         SimpleLogger().Write(logDEBUG) << "Unmapped from node " <<
    //         current_restriction.from.node
    //                                        << " of restriction";
    //         continue;
    //     }
    //     current_restriction.from.node = internal_id_iter->second;

    //     internal_id_iter = ext_to_int_id_map.find(current_restriction.via.node);
    //     if (internal_id_iter == ext_to_int_id_map.end())
    //     {
    //         SimpleLogger().Write(logDEBUG) << "Unmapped via node " <<
    //         current_restriction.via.node
    //                                        << " of restriction";
    //         continue;
    //     }

    //     current_restriction.via.node = internal_id_iter->second;

    //     internal_id_iter = ext_to_int_id_map.find(current_restriction.to.node);
    //     if (internal_id_iter == ext_to_int_id_map.end())
    //     {
    //         SimpleLogger().Write(logDEBUG) << "Unmapped to node " << current_restriction.to.node
    //                                        << " of restriction";
    //         continue;
    //     }
    //     current_restriction.to.node = internal_id_iter->second;
    // }

    EdgeWeight weight;
    NodeID source, target;
    unsigned nameID;
    int length;
    short dir; // direction (0 = open, 1 = forward, 2+ = open)
    bool is_roundabout, ignore_in_grid, is_access_restricted, is_split;
    TravelMode travel_mode;
    unsigned way_id;

    EdgeID m;
    input_stream.read(reinterpret_cast<char *>(&m), sizeof(unsigned));
    edge_list.reserve(m);
    SimpleLogger().Write() << " and " << m << " edges ";

    for (EdgeID i = 0; i < m; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&way_id), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&source), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&target), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&length), sizeof(int));
        input_stream.read(reinterpret_cast<char *>(&dir), sizeof(short));
        input_stream.read(reinterpret_cast<char *>(&weight), sizeof(int));
        input_stream.read(reinterpret_cast<char *>(&nameID), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&is_roundabout), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&ignore_in_grid), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&is_access_restricted), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&travel_mode), sizeof(TravelMode));
        input_stream.read(reinterpret_cast<char *>(&is_split), sizeof(bool));

        BOOST_ASSERT_MSG(length > 0, "loaded null length edge");
        BOOST_ASSERT_MSG(weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(0 <= dir && dir <= 2, "loaded bogus direction");

        // bool forward = true;
        // bool backward = true;
        // if (1 == dir)
        // {
        //     backward = false;
        // }
        // if (2 == dir)
        // {
        //     forward = false;
        // }

        // translate the external NodeIDs to internal IDs
        auto internal_id_iter = ext_to_int_id_map.find(source);
        if (ext_to_int_id_map.find(source) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << " unresolved source NodeID: " << source;
#endif
            continue;
        }
        source = internal_id_iter->second;
        internal_id_iter = ext_to_int_id_map.find(target);
        if (ext_to_int_id_map.find(target) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << "unresolved target NodeID : " << target;
#endif
            continue;
        }
        target = internal_id_iter->second;
        BOOST_ASSERT_MSG(source != SPECIAL_NODEID && target != SPECIAL_NODEID,
                         "nonexisting source or target");

        if (source > target)
        {
            std::swap(source, target);
            // std::swap(forward, backward);
        }

        edge_list.emplace_back(source, target);
    }
    ext_to_int_id_map.clear();

    sort(edge_list.begin(), edge_list.end());

    // for (unsigned i = 1; i < edge_list.size(); ++i)
    // {
    //     if ((edge_list[i - 1].target == edge_list[i].target) &&
    //         (edge_list[i - 1].source == edge_list[i].source))
    //     {
    //         const bool edge_flags_equivalent = (edge_list[i - 1].forward == edge_list[i].forward)
    //         &&
    //                                            (edge_list[i - 1].backward ==
    //                                            edge_list[i].backward);
    //         const bool edge_flags_are_superset1 =
    //             (edge_list[i - 1].forward && edge_list[i - 1].backward) &&
    //             (edge_list[i].forward != edge_list[i].backward);
    //         const bool edge_flags_are_superset_2 =
    //             (edge_list[i].forward && edge_list[i].backward) &&
    //             (edge_list[i - 1].forward != edge_list[i - 1].backward);

    //         if (edge_flags_equivalent)
    //         {
    //             edge_list[i].weight = std::min(edge_list[i - 1].weight, edge_list[i].weight);
    //             edge_list[i - 1].source = SPECIAL_NODEID;
    //         }
    //         else if (edge_flags_are_superset1)
    //         {
    //             if (edge_list[i - 1].weight <= edge_list[i].weight)
    //             {
    //                 // edge i-1 is smaller and goes in both directions. Throw away the other edge
    //                 edge_list[i].source = SPECIAL_NODEID;
    //             }
    //             else
    //             {
    //                 // edge i-1 is open in both directions, but edge i is smaller in one
    //                 direction.
    //                 // Close edge i-1 in this direction
    //                 edge_list[i - 1].forward = !edge_list[i].forward;
    //                 edge_list[i - 1].backward = !edge_list[i].backward;
    //             }
    //         }
    //         else if (edge_flags_are_superset_2)
    //         {
    //             if (edge_list[i - 1].weight <= edge_list[i].weight)
    //             {
    //                 // edge i-1 is smaller for one direction. edge i is open in both. close edge
    //                 i
    //                 // in the other direction
    //                 edge_list[i].forward = !edge_list[i - 1].forward;
    //                 edge_list[i].backward = !edge_list[i - 1].backward;
    //             }
    //             else
    //             {
    //                 // edge i is smaller and goes in both direction. Throw away edge i-1
    //                 edge_list[i - 1].source = SPECIAL_NODEID;
    //             }
    //         }
    //     }
    // }
    // const auto new_end_iter =
    //     std::remove_if(edge_list.begin(), edge_list.end(), [](const EdgeT &edge)
    //                    {
    //                        return edge.source == SPECIAL_NODEID || edge.target == SPECIAL_NODEID;
    //                    });
    // edge_list.erase(new_end_iter, edge_list.end()); // remove excess candidates.
    // edge_list.shrink_to_fit();
    SimpleLogger().Write() << "Graph loaded ok and has " << edge_list.size() << " edges";
    return n;
}

template <typename EdgeT>
NodeID readBinaryOSRMGraphFromStream(std::istream &input_stream,
                                     std::vector<EdgeT> &edge_list,
                                     std::vector<NodeID> &barrier_node_list,
                                     std::vector<NodeID> &traffic_light_node_list,
                                     std::vector<QueryNode> *int_to_ext_node_id_map,
                                     std::vector<TurnRestriction> &restriction_list)
{
    const FingerPrint fingerprint_orig;
    FingerPrint fingerprint_loaded;
    input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));

    if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".osrm was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    std::unordered_map<NodeID, NodeID> ext_to_int_id_map;

    NodeID n;
    input_stream.read(reinterpret_cast<char *>(&n), sizeof(NodeID));
    SimpleLogger().Write() << "Importing n = " << n << " nodes ";

    ExternalMemoryNode current_node;
    for (NodeID i = 0; i < n; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&current_node), sizeof(ExternalMemoryNode));
        int_to_ext_node_id_map->emplace_back(current_node.lat, current_node.lon,
                                             current_node.node_id);
        ext_to_int_id_map.emplace(current_node.node_id, i);
        if (current_node.barrier)
        {
            barrier_node_list.emplace_back(i);
        }
        if (current_node.traffic_lights)
        {
            traffic_light_node_list.emplace_back(i);
        }
    }

    // tighten vector sizes
    barrier_node_list.shrink_to_fit();
    traffic_light_node_list.shrink_to_fit();

    // renumber nodes in turn restrictions
    for (TurnRestriction &current_restriction : restriction_list)
    {
        auto internal_id_iter = ext_to_int_id_map.find(current_restriction.from.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped from node " << current_restriction.from.node
                                           << " of restriction";
            continue;
        }
        current_restriction.from.node = internal_id_iter->second;

        internal_id_iter = ext_to_int_id_map.find(current_restriction.via.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped via node " << current_restriction.via.node
                                           << " of restriction";
            continue;
        }

        current_restriction.via.node = internal_id_iter->second;

        internal_id_iter = ext_to_int_id_map.find(current_restriction.to.node);
        if (internal_id_iter == ext_to_int_id_map.end())
        {
            SimpleLogger().Write(logDEBUG) << "Unmapped to node " << current_restriction.to.node
                                           << " of restriction";
            continue;
        }
        current_restriction.to.node = internal_id_iter->second;
    }

    EdgeWeight weight;
    NodeID source, target;
    unsigned nameID;
    int length;
    unsigned way_id;
    short dir; // direction (0 = open, 1 = forward, 2+ = open)
    bool is_roundabout, ignore_in_grid, is_access_restricted, is_split;
    TravelMode travel_mode;

    EdgeID m;
    input_stream.read(reinterpret_cast<char *>(&m), sizeof(unsigned));
    edge_list.reserve(m);
    SimpleLogger().Write() << " and " << m << " edges ";

    for (EdgeID i = 0; i < m; ++i)
    {
        input_stream.read(reinterpret_cast<char *>(&way_id), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&source), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&target), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&length), sizeof(int));
        input_stream.read(reinterpret_cast<char *>(&dir), sizeof(short));
        input_stream.read(reinterpret_cast<char *>(&weight), sizeof(int));
        input_stream.read(reinterpret_cast<char *>(&nameID), sizeof(unsigned));
        input_stream.read(reinterpret_cast<char *>(&is_roundabout), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&ignore_in_grid), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&is_access_restricted), sizeof(bool));
        input_stream.read(reinterpret_cast<char *>(&travel_mode), sizeof(TravelMode));
        input_stream.read(reinterpret_cast<char *>(&is_split), sizeof(bool));

        BOOST_ASSERT_MSG(length > 0, "loaded null length edge");
        BOOST_ASSERT_MSG(weight > 0, "loaded null weight");
        BOOST_ASSERT_MSG(0 <= dir && dir <= 2, "loaded bogus direction");

        bool forward = true;
        bool backward = true;
        if (1 == dir)
        {
            backward = false;
        }
        if (2 == dir)
        {
            forward = false;
        }

        // translate the external NodeIDs to internal IDs
        auto internal_id_iter = ext_to_int_id_map.find(source);
        if (ext_to_int_id_map.find(source) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << " unresolved source NodeID: " << source;
#endif
            continue;
        }
        source = internal_id_iter->second;
        internal_id_iter = ext_to_int_id_map.find(target);
        if (ext_to_int_id_map.find(target) == ext_to_int_id_map.end())
        {
#ifndef NDEBUG
            SimpleLogger().Write(logWARNING) << "unresolved target NodeID : " << target;
#endif
            continue;
        }
        target = internal_id_iter->second;
        BOOST_ASSERT_MSG(source != SPECIAL_NODEID && target != SPECIAL_NODEID,
                         "nonexisting source or target");

        if (source > target)
        {
            std::swap(source, target);
            std::swap(forward, backward);
        }

        edge_list.emplace_back(way_id, source, target, nameID, weight, forward, backward, is_roundabout,
                               ignore_in_grid, is_access_restricted, travel_mode, is_split);
    }
    ext_to_int_id_map.clear();

    std::sort(edge_list.begin(), edge_list.end());

    for (unsigned i = 1; i < edge_list.size(); ++i)
    {
        if ((edge_list[i - 1].target == edge_list[i].target) &&
            (edge_list[i - 1].source == edge_list[i].source))
        {
            const bool edge_flags_equivalent = (edge_list[i - 1].forward == edge_list[i].forward) &&
                                               (edge_list[i - 1].backward == edge_list[i].backward);
            const bool edge_flags_are_superset1 =
                (edge_list[i - 1].forward && edge_list[i - 1].backward) &&
                (edge_list[i].forward != edge_list[i].backward);
            const bool edge_flags_are_superset_2 =
                (edge_list[i].forward && edge_list[i].backward) &&
                (edge_list[i - 1].forward != edge_list[i - 1].backward);

            if (edge_flags_equivalent)
            {
                edge_list[i].weight = std::min(edge_list[i - 1].weight, edge_list[i].weight);
                edge_list[i - 1].source = SPECIAL_NODEID;
            }
            else if (edge_flags_are_superset1)
            {
                if (edge_list[i - 1].weight <= edge_list[i].weight)
                {
                    // edge i-1 is smaller and goes in both directions. Throw away the other edge
                    edge_list[i].source = SPECIAL_NODEID;
                }
                else
                {
                    // edge i-1 is open in both directions, but edge i is smaller in one direction.
                    // Close edge i-1 in this direction
                    edge_list[i - 1].forward = !edge_list[i].forward;
                    edge_list[i - 1].backward = !edge_list[i].backward;
                }
            }
            else if (edge_flags_are_superset_2)
            {
                if (edge_list[i - 1].weight <= edge_list[i].weight)
                {
                    // edge i-1 is smaller for one direction. edge i is open in both. close edge i
                    // in the other direction
                    edge_list[i].forward = !edge_list[i - 1].forward;
                    edge_list[i].backward = !edge_list[i - 1].backward;
                }
                else
                {
                    // edge i is smaller and goes in both direction. Throw away edge i-1
                    edge_list[i - 1].source = SPECIAL_NODEID;
                }
            }
        }
    }
    const auto new_end_iter =
        std::remove_if(edge_list.begin(), edge_list.end(), [](const EdgeT &edge)
                       {
                           return edge.source == SPECIAL_NODEID || edge.target == SPECIAL_NODEID;
                       });
    edge_list.erase(new_end_iter, edge_list.end()); // remove excess candidates.
    edge_list.shrink_to_fit();
    SimpleLogger().Write() << "Graph loaded ok and has " << edge_list.size() << " edges";
    return n;
}

template <typename NodeT, typename EdgeT>
unsigned readHSGRFromStream(const boost::filesystem::path &hsgr_file,
                            std::vector<NodeT> &node_list,
                            std::vector<EdgeT> &edge_list,
                            unsigned *check_sum)
{
    if (!boost::filesystem::exists(hsgr_file))
    {
        throw osrm::exception("hsgr file does not exist");
    }
    if (0 == boost::filesystem::file_size(hsgr_file))
    {
        throw osrm::exception("hsgr file is empty");
    }

    boost::filesystem::ifstream hsgr_input_stream(hsgr_file, std::ios::binary);

    FingerPrint fingerprint_loaded, fingerprint_orig;
    hsgr_input_stream.read(reinterpret_cast<char *>(&fingerprint_loaded), sizeof(FingerPrint));
    if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
    {
        SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build.\n"
                                            "Reprocess to get rid of this warning.";
    }

    unsigned number_of_nodes = 0;
    unsigned number_of_edges = 0;
    hsgr_input_stream.read(reinterpret_cast<char *>(check_sum), sizeof(unsigned));
    hsgr_input_stream.read(reinterpret_cast<char *>(&number_of_nodes), sizeof(unsigned));
    BOOST_ASSERT_MSG(0 != number_of_nodes, "number of nodes is zero");
    hsgr_input_stream.read(reinterpret_cast<char *>(&number_of_edges), sizeof(unsigned));

    SimpleLogger().Write() << "number_of_nodes: " << number_of_nodes
                           << ", number_of_edges: " << number_of_edges;

    // BOOST_ASSERT_MSG( 0 != number_of_edges, "number of edges is zero");
    node_list.resize(number_of_nodes);
    hsgr_input_stream.read(reinterpret_cast<char *>(&node_list[0]),
                           number_of_nodes * sizeof(NodeT));

    edge_list.resize(number_of_edges);
    if (number_of_edges > 0)
    {
        hsgr_input_stream.read(reinterpret_cast<char *>(&edge_list[0]),
                               number_of_edges * sizeof(EdgeT));
    }
    hsgr_input_stream.close();

    return number_of_nodes;
}

#endif // GRAPH_LOADER_HPP
