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

#include "data_structures/original_edge_data.hpp"
#include "data_structures/range_table.hpp"
#include "data_structures/query_edge.hpp"
#include "data_structures/query_node.hpp"
#include "data_structures/shared_memory_factory.hpp"
#include "data_structures/shared_memory_vector_wrapper.hpp"
#include "data_structures/static_graph.hpp"
#include "data_structures/static_rtree.hpp"
#include "data_structures/travel_mode.hpp"
#include "data_structures/turn_instructions.hpp"
#include "server/data_structures/datafacade_base.hpp"
#include "server/data_structures/shared_datatype.hpp"
#include "server/data_structures/shared_barriers.hpp"
#include "util/boost_filesystem_2_fix.hpp"
#include "util/datastore_options.hpp"
#include "util/simple_logger.hpp"
#include "util/osrm_exception.hpp"
#include "util/fingerprint.hpp"
#include "typedefs.h"

#include <osrm/coordinate.hpp>
#include <osrm/server_paths.hpp>

using RTreeLeaf = BaseDataFacade<QueryEdge::EdgeData>::RTreeLeaf;
using RTreeNode = StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>::TreeNode;
using QueryGraph = StaticGraph<QueryEdge::EdgeData>;

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/seek.hpp>

#include <cstdint>

#include <fstream>
#include <string>

// delete a shared memory region. report warning if it could not be deleted
void delete_region(const SharedDataType region)
{
    if (SharedMemory::RegionExists(region) && !SharedMemory::Remove(region))
    {
        const std::string name = [&]
        {
            switch (region)
            {
            case CURRENT_REGIONS:
                return "CURRENT_REGIONS";
            case LAYOUT_1:
                return "LAYOUT_1";
            case DATA_1:
                return "DATA_1";
            case LAYOUT_2:
                return "LAYOUT_2";
            case DATA_2:
                return "DATA_2";
            case LAYOUT_NONE:
                return "LAYOUT_NONE";
            default: // DATA_NONE:
                return "DATA_NONE";
            }
        }();

        SimpleLogger().Write(logWARNING) << "could not delete shared memory region " << name;
    }
}

int main(const int argc, const char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    SharedBarriers barrier;

    try
    {
#ifdef __linux__
        // try to disable swapping on Linux
        const bool lock_flags = MCL_CURRENT | MCL_FUTURE;
        if (-1 == mlockall(lock_flags))
        {
            SimpleLogger().Write(logWARNING) << "Process " << argv[0]
                                             << " could not request RAM lock";
        }
#endif
        try
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
                barrier.pending_update_mutex);
        }
        catch (...)
        {
            // hard unlock in case of any exception.
            barrier.pending_update_mutex.unlock();
        }
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }

    try
    {
        SimpleLogger().Write(logDEBUG) << "Checking input parameters";

        ServerPaths server_paths;
        if (!GenerateDataStoreOptions(argc, argv, server_paths))
        {
            return 0;
        }

        if (server_paths.find("hsgrdata") == server_paths.end())
        {
            throw osrm::exception("no hsgr file found");
        }
        if (server_paths.find("ramindex") == server_paths.end())
        {
            throw osrm::exception("no ram index file found");
        }
        if (server_paths.find("fileindex") == server_paths.end())
        {
            throw osrm::exception("no leaf index file found");
        }
        if (server_paths.find("nodesdata") == server_paths.end())
        {
            throw osrm::exception("no nodes file found");
        }
        if (server_paths.find("edgesdata") == server_paths.end())
        {
            throw osrm::exception("no edges file found");
        }
        if (server_paths.find("namesdata") == server_paths.end())
        {
            throw osrm::exception("no names file found");
        }
        if (server_paths.find("geometry") == server_paths.end())
        {
            throw osrm::exception("no geometry file found");
        }

        ServerPaths::const_iterator paths_iterator = server_paths.find("hsgrdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &hsgr_path = paths_iterator->second;
        paths_iterator = server_paths.find("timestamp");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &timestamp_path = paths_iterator->second;
        paths_iterator = server_paths.find("ramindex");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &ram_index_path = paths_iterator->second;
        paths_iterator = server_paths.find("fileindex");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path index_file_path_absolute =
            boost::filesystem::portable_canonical(paths_iterator->second);
        const std::string &file_index_path = index_file_path_absolute.string();
        paths_iterator = server_paths.find("nodesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &nodes_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("edgesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &edges_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("namesdata");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &names_data_path = paths_iterator->second;
        paths_iterator = server_paths.find("geometry");
        BOOST_ASSERT(server_paths.end() != paths_iterator);
        BOOST_ASSERT(!paths_iterator->second.empty());
        const boost::filesystem::path &geometries_data_path = paths_iterator->second;

        // determine segment to use
        bool segment2_in_use = SharedMemory::RegionExists(LAYOUT_2);
        const SharedDataType layout_region = [&]
        {
            return segment2_in_use ? LAYOUT_1 : LAYOUT_2;
        }();
        const SharedDataType data_region = [&]
        {
            return segment2_in_use ? DATA_1 : DATA_2;
        }();
        const SharedDataType previous_layout_region = [&]
        {
            return segment2_in_use ? LAYOUT_2 : LAYOUT_1;
        }();
        const SharedDataType previous_data_region = [&]
        {
            return segment2_in_use ? DATA_2 : DATA_1;
        }();

        // Allocate a memory layout in shared memory, deallocate previous
        SharedMemory *layout_memory =
            SharedMemoryFactory::Get(layout_region, sizeof(SharedDataLayout));
        SharedDataLayout *shared_layout_ptr = static_cast<SharedDataLayout *>(layout_memory->Ptr());
        shared_layout_ptr = new (layout_memory->Ptr()) SharedDataLayout();

        shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::FILE_INDEX_PATH,
                                              file_index_path.length() + 1);

        // collect number of elements to store in shared memory object
        SimpleLogger().Write() << "load names from: " << names_data_path;
        // number of entries in name index
        boost::filesystem::ifstream name_stream(names_data_path, std::ios::binary);
        unsigned name_blocks = 0;
        name_stream.read((char *)&name_blocks, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::NAME_OFFSETS, name_blocks);
        shared_layout_ptr->SetBlockSize<typename RangeTable<16, true>::BlockT>(
            SharedDataLayout::NAME_BLOCKS, name_blocks);
        SimpleLogger().Write() << "name offsets size: " << name_blocks;
        BOOST_ASSERT_MSG(0 != name_blocks, "name file broken");

        unsigned number_of_chars = 0;
        name_stream.read((char *)&number_of_chars, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::NAME_CHAR_LIST, number_of_chars);

        // Loading information for original edges
        boost::filesystem::ifstream edges_input_stream(edges_data_path, std::ios::binary);
        unsigned number_of_original_edges = 0;
        edges_input_stream.read((char *)&number_of_original_edges, sizeof(unsigned));

        // note: settings this all to the same size is correct, we extract them from the same struct
        shared_layout_ptr->SetBlockSize<NodeID>(SharedDataLayout::VIA_NODE_LIST,
                                                number_of_original_edges);
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::NAME_ID_LIST,
                                                  number_of_original_edges);
        shared_layout_ptr->SetBlockSize<TravelMode>(SharedDataLayout::TRAVEL_MODE,
                                                    number_of_original_edges);
        shared_layout_ptr->SetBlockSize<TurnInstruction>(SharedDataLayout::TURN_INSTRUCTION,
                                                         number_of_original_edges);
        // note: there are 32 geometry indicators in one unsigned block
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::GEOMETRIES_INDICATORS,
                                                  number_of_original_edges);

        boost::filesystem::ifstream hsgr_input_stream(hsgr_path, std::ios::binary);

        FingerPrint fingerprint_loaded, fingerprint_orig;
        hsgr_input_stream.read((char *)&fingerprint_loaded, sizeof(FingerPrint));
        if (fingerprint_loaded.TestGraphUtil(fingerprint_orig))
        {
            SimpleLogger().Write(logDEBUG) << "Fingerprint checked out ok";
        }
        else
        {
            SimpleLogger().Write(logWARNING) << ".hsgr was prepared with different build. "
                                                "Reprocess to get rid of this warning.";
        }

        // load checksum
        unsigned checksum = 0;
        hsgr_input_stream.read((char *)&checksum, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::HSGR_CHECKSUM, 1);
        // load graph node size
        unsigned number_of_graph_nodes = 0;
        hsgr_input_stream.read((char *)&number_of_graph_nodes, sizeof(unsigned));

        BOOST_ASSERT_MSG((0 != number_of_graph_nodes), "number of nodes is zero");
        shared_layout_ptr->SetBlockSize<QueryGraph::NodeArrayEntry>(
            SharedDataLayout::GRAPH_NODE_LIST, number_of_graph_nodes);

        // load graph edge size
        unsigned number_of_graph_edges = 0;
        hsgr_input_stream.read((char *)&number_of_graph_edges, sizeof(unsigned));
        // BOOST_ASSERT_MSG(0 != number_of_graph_edges, "number of graph edges is zero");
        shared_layout_ptr->SetBlockSize<QueryGraph::EdgeArrayEntry>(
            SharedDataLayout::GRAPH_EDGE_LIST, number_of_graph_edges);

        // load rsearch tree size
        boost::filesystem::ifstream tree_node_file(ram_index_path, std::ios::binary);

        uint32_t tree_size = 0;
        tree_node_file.read((char *)&tree_size, sizeof(uint32_t));
        shared_layout_ptr->SetBlockSize<RTreeNode>(SharedDataLayout::R_SEARCH_TREE, tree_size);

        // load timestamp size
        std::string m_timestamp;
        if (boost::filesystem::exists(timestamp_path))
        {
            boost::filesystem::ifstream timestamp_stream(timestamp_path);
            if (!timestamp_stream)
            {
                SimpleLogger().Write(logWARNING) << timestamp_path
                                                 << " not found. setting to default";
            }
            else
            {
                getline(timestamp_stream, m_timestamp);
                timestamp_stream.close();
            }
        }
        if (m_timestamp.empty())
        {
            m_timestamp = "n/a";
        }
        if (25 < m_timestamp.length())
        {
            m_timestamp.resize(25);
        }
        shared_layout_ptr->SetBlockSize<char>(SharedDataLayout::TIMESTAMP, m_timestamp.length());

        // load coordinate size
        boost::filesystem::ifstream nodes_input_stream(nodes_data_path, std::ios::binary);
        unsigned coordinate_list_size = 0;
        nodes_input_stream.read((char *)&coordinate_list_size, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<FixedPointCoordinate>(SharedDataLayout::COORDINATE_LIST,
                                                              coordinate_list_size);

        // load geometries sizes
        std::ifstream geometry_input_stream(geometries_data_path.string().c_str(),
                                            std::ios::binary);
        unsigned number_of_geometries_indices = 0;
        unsigned number_of_compressed_geometries = 0;

        geometry_input_stream.read((char *)&number_of_geometries_indices, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::GEOMETRIES_INDEX,
                                                  number_of_geometries_indices);
        boost::iostreams::seek(geometry_input_stream,
                               number_of_geometries_indices * sizeof(unsigned), BOOST_IOS::cur);
        geometry_input_stream.read((char *)&number_of_compressed_geometries, sizeof(unsigned));
        shared_layout_ptr->SetBlockSize<unsigned>(SharedDataLayout::GEOMETRIES_LIST,
                                                  number_of_compressed_geometries);
        // allocate shared memory block
        SimpleLogger().Write() << "allocating shared memory of "
                               << shared_layout_ptr->GetSizeOfLayout() << " bytes";
        SharedMemory *shared_memory =
            SharedMemoryFactory::Get(data_region, shared_layout_ptr->GetSizeOfLayout());
        char *shared_memory_ptr = static_cast<char *>(shared_memory->Ptr());

        // read actual data into shared memory object //

        // hsgr checksum
        unsigned *checksum_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::HSGR_CHECKSUM);
        *checksum_ptr = checksum;

        // ram index file name
        char *file_index_path_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
            shared_memory_ptr, SharedDataLayout::FILE_INDEX_PATH);
        // make sure we have 0 ending
        std::fill(file_index_path_ptr,
                  file_index_path_ptr +
                      shared_layout_ptr->GetBlockSize(SharedDataLayout::FILE_INDEX_PATH),
                  0);
        std::copy(file_index_path.begin(), file_index_path.end(), file_index_path_ptr);

        // Loading street names
        unsigned *name_offsets_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::NAME_OFFSETS);
        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_OFFSETS) > 0)
        {
            name_stream.read((char *)name_offsets_ptr,
                             shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_OFFSETS));
        }

        unsigned *name_blocks_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::NAME_BLOCKS);
        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_BLOCKS) > 0)
        {
            name_stream.read((char *)name_blocks_ptr,
                             shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_BLOCKS));
        }

        char *name_char_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
            shared_memory_ptr, SharedDataLayout::NAME_CHAR_LIST);
        unsigned temp_length;
        name_stream.read((char *)&temp_length, sizeof(unsigned));

        BOOST_ASSERT_MSG(temp_length ==
                             shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST),
                         "Name file corrupted!");

        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST) > 0)
        {
            name_stream.read(name_char_ptr,
                             shared_layout_ptr->GetBlockSize(SharedDataLayout::NAME_CHAR_LIST));
        }

        name_stream.close();

        // load original edge information
        NodeID *via_node_ptr = shared_layout_ptr->GetBlockPtr<NodeID, true>(
            shared_memory_ptr, SharedDataLayout::VIA_NODE_LIST);

        unsigned *name_id_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::NAME_ID_LIST);

        TravelMode *travel_mode_ptr = shared_layout_ptr->GetBlockPtr<TravelMode, true>(
            shared_memory_ptr, SharedDataLayout::TRAVEL_MODE);

        TurnInstruction *turn_instructions_ptr =
            shared_layout_ptr->GetBlockPtr<TurnInstruction, true>(
                shared_memory_ptr, SharedDataLayout::TURN_INSTRUCTION);

        unsigned *geometries_indicator_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::GEOMETRIES_INDICATORS);

        OriginalEdgeData current_edge_data;
        for (unsigned i = 0; i < number_of_original_edges; ++i)
        {
            edges_input_stream.read((char *)&(current_edge_data), sizeof(OriginalEdgeData));
            via_node_ptr[i] = current_edge_data.via_node;
            name_id_ptr[i] = current_edge_data.name_id;
            travel_mode_ptr[i] = current_edge_data.travel_mode;
            turn_instructions_ptr[i] = current_edge_data.turn_instruction;

            const unsigned bucket = i / 32;
            const unsigned offset = i % 32;
            const unsigned value = [&]
            {
                unsigned return_value = 0;
                if (0 != offset)
                {
                    return_value = geometries_indicator_ptr[bucket];
                }
                return return_value;
            }();
            if (current_edge_data.compressed_geometry)
            {
                geometries_indicator_ptr[bucket] = (value | (1 << offset));
            }
        }
        edges_input_stream.close();

        // load compressed geometry
        unsigned temporary_value;
        unsigned *geometries_index_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::GEOMETRIES_INDEX);
        geometry_input_stream.seekg(0, geometry_input_stream.beg);
        geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
        BOOST_ASSERT(temporary_value ==
                     shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_INDEX]);

        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_INDEX) > 0)
        {
            geometry_input_stream.read(
                (char *)geometries_index_ptr,
                shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_INDEX));
        }
        unsigned *geometries_list_ptr = shared_layout_ptr->GetBlockPtr<unsigned, true>(
            shared_memory_ptr, SharedDataLayout::GEOMETRIES_LIST);

        geometry_input_stream.read((char *)&temporary_value, sizeof(unsigned));
        BOOST_ASSERT(temporary_value ==
                     shared_layout_ptr->num_entries[SharedDataLayout::GEOMETRIES_LIST]);

        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_LIST) > 0)
        {
            geometry_input_stream.read(
                (char *)geometries_list_ptr,
                shared_layout_ptr->GetBlockSize(SharedDataLayout::GEOMETRIES_LIST));
        }

        // Loading list of coordinates
        FixedPointCoordinate *coordinates_ptr =
            shared_layout_ptr->GetBlockPtr<FixedPointCoordinate, true>(
                shared_memory_ptr, SharedDataLayout::COORDINATE_LIST);

        QueryNode current_node;
        for (unsigned i = 0; i < coordinate_list_size; ++i)
        {
            nodes_input_stream.read((char *)&current_node, sizeof(QueryNode));
            coordinates_ptr[i] = FixedPointCoordinate(current_node.lat, current_node.lon);
        }
        nodes_input_stream.close();

        // store timestamp
        char *timestamp_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
            shared_memory_ptr, SharedDataLayout::TIMESTAMP);
        std::copy(m_timestamp.c_str(), m_timestamp.c_str() + m_timestamp.length(), timestamp_ptr);

        // store search tree portion of rtree
        char *rtree_ptr = shared_layout_ptr->GetBlockPtr<char, true>(
            shared_memory_ptr, SharedDataLayout::R_SEARCH_TREE);

        if (tree_size > 0)
        {
            tree_node_file.read(rtree_ptr, sizeof(RTreeNode) * tree_size);
        }
        tree_node_file.close();

        // load the nodes of the search graph
        QueryGraph::NodeArrayEntry *graph_node_list_ptr =
            shared_layout_ptr->GetBlockPtr<QueryGraph::NodeArrayEntry, true>(
                shared_memory_ptr, SharedDataLayout::GRAPH_NODE_LIST);
        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_NODE_LIST) > 0)
        {
            hsgr_input_stream.read(
                (char *)graph_node_list_ptr,
                shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_NODE_LIST));
        }

        // load the edges of the search graph
        QueryGraph::EdgeArrayEntry *graph_edge_list_ptr =
            shared_layout_ptr->GetBlockPtr<QueryGraph::EdgeArrayEntry, true>(
                shared_memory_ptr, SharedDataLayout::GRAPH_EDGE_LIST);
        if (shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_EDGE_LIST) > 0)
        {
            hsgr_input_stream.read(
                (char *)graph_edge_list_ptr,
                shared_layout_ptr->GetBlockSize(SharedDataLayout::GRAPH_EDGE_LIST));
        }
        hsgr_input_stream.close();

        // acquire lock
        SharedMemory *data_type_memory =
            SharedMemoryFactory::Get(CURRENT_REGIONS, sizeof(SharedDataTimestamp), true, false);
        SharedDataTimestamp *data_timestamp_ptr =
            static_cast<SharedDataTimestamp *>(data_type_memory->Ptr());

        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
            barrier.query_mutex);

        // notify all processes that were waiting for this condition
        if (0 < barrier.number_of_queries)
        {
            barrier.no_running_queries_condition.wait(query_lock);
        }

        data_timestamp_ptr->layout = layout_region;
        data_timestamp_ptr->data = data_region;
        data_timestamp_ptr->timestamp += 1;
        delete_region(previous_data_region);
        delete_region(previous_layout_region);
        SimpleLogger().Write() << "all data loaded";

        shared_layout_ptr->PrintInformation();
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "caught exception: " << e.what();
    }

    return 0;
}
