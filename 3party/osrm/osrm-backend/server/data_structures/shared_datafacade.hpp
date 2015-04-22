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

#ifndef SHARED_DATAFACADE_HPP
#define SHARED_DATAFACADE_HPP

// implements all data storage when shared memory _IS_ used

#include "datafacade_base.hpp"
#include "shared_datatype.hpp"

#include "../../data_structures/range_table.hpp"
#include "../../data_structures/static_graph.hpp"
#include "../../data_structures/static_rtree.hpp"
#include "../../util/boost_filesystem_2_fix.hpp"
#include "../../util/make_unique.hpp"
#include "../../util/simple_logger.hpp"

#include <algorithm>
#include <limits>
#include <memory>

template <class EdgeDataT> class SharedDataFacade final : public BaseDataFacade<EdgeDataT>
{

  private:
    using EdgeData = EdgeDataT;
    using super = BaseDataFacade<EdgeData>;
    using QueryGraph = StaticGraph<EdgeData, true>;
    using GraphNode = typename StaticGraph<EdgeData, true>::NodeArrayEntry;
    using GraphEdge = typename StaticGraph<EdgeData, true>::EdgeArrayEntry;
    using NameIndexBlock = typename RangeTable<16, true>::BlockT;
    using InputEdge = typename QueryGraph::InputEdge;
    using RTreeLeaf = typename super::RTreeLeaf;
    using SharedRTree = StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, true>::vector, true>;
    using TimeStampedRTreePair = std::pair<unsigned, std::shared_ptr<SharedRTree>>;
    using RTreeNode = typename SharedRTree::TreeNode;

    SharedDataLayout *data_layout;
    char *shared_memory;
    SharedDataTimestamp *data_timestamp_ptr;

    SharedDataType CURRENT_LAYOUT;
    SharedDataType CURRENT_DATA;
    unsigned CURRENT_TIMESTAMP;

    unsigned m_check_sum;
    std::unique_ptr<QueryGraph> m_query_graph;
    std::unique_ptr<SharedMemory> m_layout_memory;
    std::unique_ptr<SharedMemory> m_large_memory;
    std::string m_timestamp;

    std::shared_ptr<ShM<FixedPointCoordinate, true>::vector> m_coordinate_list;
    ShM<NodeID, true>::vector m_via_node_list;
    ShM<unsigned, true>::vector m_name_ID_list;
    ShM<TurnInstruction, true>::vector m_turn_instruction_list;
    ShM<TravelMode, true>::vector m_travel_mode_list;
    ShM<char, true>::vector m_names_char_list;
    ShM<unsigned, true>::vector m_name_begin_indices;
    ShM<bool, true>::vector m_edge_is_compressed;
    ShM<unsigned, true>::vector m_geometry_indices;
    ShM<unsigned, true>::vector m_geometry_list;

    boost::thread_specific_ptr<std::pair<unsigned, std::shared_ptr<SharedRTree>>> m_static_rtree;
    boost::filesystem::path file_index_path;

    std::shared_ptr<RangeTable<16, true>> m_name_table;

    void LoadChecksum()
    {
        m_check_sum =
            *data_layout->GetBlockPtr<unsigned>(shared_memory, SharedDataLayout::HSGR_CHECKSUM);
        SimpleLogger().Write() << "set checksum: " << m_check_sum;
    }

    void LoadTimestamp()
    {
        char *timestamp_ptr =
            data_layout->GetBlockPtr<char>(shared_memory, SharedDataLayout::TIMESTAMP);
        m_timestamp.resize(data_layout->GetBlockSize(SharedDataLayout::TIMESTAMP));
        std::copy(timestamp_ptr,
                  timestamp_ptr + data_layout->GetBlockSize(SharedDataLayout::TIMESTAMP),
                  m_timestamp.begin());
    }

    void LoadRTree()
    {
        BOOST_ASSERT_MSG(!m_coordinate_list->empty(), "coordinates must be loaded before r-tree");

        RTreeNode *tree_ptr =
            data_layout->GetBlockPtr<RTreeNode>(shared_memory, SharedDataLayout::R_SEARCH_TREE);
        m_static_rtree.reset(new TimeStampedRTreePair(
            CURRENT_TIMESTAMP,
            osrm::make_unique<SharedRTree>(
                tree_ptr, data_layout->num_entries[SharedDataLayout::R_SEARCH_TREE],
                file_index_path, m_coordinate_list)));
    }

    void LoadGraph()
    {
        GraphNode *graph_nodes_ptr =
            data_layout->GetBlockPtr<GraphNode>(shared_memory, SharedDataLayout::GRAPH_NODE_LIST);

        GraphEdge *graph_edges_ptr =
            data_layout->GetBlockPtr<GraphEdge>(shared_memory, SharedDataLayout::GRAPH_EDGE_LIST);

        typename ShM<GraphNode, true>::vector node_list(
            graph_nodes_ptr, data_layout->num_entries[SharedDataLayout::GRAPH_NODE_LIST]);
        typename ShM<GraphEdge, true>::vector edge_list(
            graph_edges_ptr, data_layout->num_entries[SharedDataLayout::GRAPH_EDGE_LIST]);
        m_query_graph.reset(new QueryGraph(node_list, edge_list));
    }

    void LoadNodeAndEdgeInformation()
    {

        FixedPointCoordinate *coordinate_list_ptr = data_layout->GetBlockPtr<FixedPointCoordinate>(
            shared_memory, SharedDataLayout::COORDINATE_LIST);
        m_coordinate_list = osrm::make_unique<ShM<FixedPointCoordinate, true>::vector>(
            coordinate_list_ptr, data_layout->num_entries[SharedDataLayout::COORDINATE_LIST]);

        TravelMode *travel_mode_list_ptr =
            data_layout->GetBlockPtr<TravelMode>(shared_memory, SharedDataLayout::TRAVEL_MODE);
        typename ShM<TravelMode, true>::vector travel_mode_list(
            travel_mode_list_ptr, data_layout->num_entries[SharedDataLayout::TRAVEL_MODE]);
        m_travel_mode_list.swap(travel_mode_list);

        TurnInstruction *turn_instruction_list_ptr = data_layout->GetBlockPtr<TurnInstruction>(
            shared_memory, SharedDataLayout::TURN_INSTRUCTION);
        typename ShM<TurnInstruction, true>::vector turn_instruction_list(
            turn_instruction_list_ptr,
            data_layout->num_entries[SharedDataLayout::TURN_INSTRUCTION]);
        m_turn_instruction_list.swap(turn_instruction_list);

        unsigned *name_id_list_ptr =
            data_layout->GetBlockPtr<unsigned>(shared_memory, SharedDataLayout::NAME_ID_LIST);
        typename ShM<unsigned, true>::vector name_id_list(
            name_id_list_ptr, data_layout->num_entries[SharedDataLayout::NAME_ID_LIST]);
        m_name_ID_list.swap(name_id_list);
    }

    void LoadViaNodeList()
    {
        NodeID *via_node_list_ptr =
            data_layout->GetBlockPtr<NodeID>(shared_memory, SharedDataLayout::VIA_NODE_LIST);
        typename ShM<NodeID, true>::vector via_node_list(
            via_node_list_ptr, data_layout->num_entries[SharedDataLayout::VIA_NODE_LIST]);
        m_via_node_list.swap(via_node_list);
    }

    void LoadNames()
    {
        unsigned *offsets_ptr =
            data_layout->GetBlockPtr<unsigned>(shared_memory, SharedDataLayout::NAME_OFFSETS);
        NameIndexBlock *blocks_ptr =
            data_layout->GetBlockPtr<NameIndexBlock>(shared_memory, SharedDataLayout::NAME_BLOCKS);
        typename ShM<unsigned, true>::vector name_offsets(
            offsets_ptr, data_layout->num_entries[SharedDataLayout::NAME_OFFSETS]);
        typename ShM<NameIndexBlock, true>::vector name_blocks(
            blocks_ptr, data_layout->num_entries[SharedDataLayout::NAME_BLOCKS]);

        char *names_list_ptr =
            data_layout->GetBlockPtr<char>(shared_memory, SharedDataLayout::NAME_CHAR_LIST);
        typename ShM<char, true>::vector names_char_list(
            names_list_ptr, data_layout->num_entries[SharedDataLayout::NAME_CHAR_LIST]);
        m_name_table = osrm::make_unique<RangeTable<16, true>>(
            name_offsets, name_blocks, static_cast<unsigned>(names_char_list.size()));

        m_names_char_list.swap(names_char_list);
    }

    void LoadGeometries()
    {
        unsigned *geometries_compressed_ptr = data_layout->GetBlockPtr<unsigned>(
            shared_memory, SharedDataLayout::GEOMETRIES_INDICATORS);
        typename ShM<bool, true>::vector edge_is_compressed(
            geometries_compressed_ptr,
            data_layout->num_entries[SharedDataLayout::GEOMETRIES_INDICATORS]);
        m_edge_is_compressed.swap(edge_is_compressed);

        unsigned *geometries_index_ptr =
            data_layout->GetBlockPtr<unsigned>(shared_memory, SharedDataLayout::GEOMETRIES_INDEX);
        typename ShM<unsigned, true>::vector geometry_begin_indices(
            geometries_index_ptr, data_layout->num_entries[SharedDataLayout::GEOMETRIES_INDEX]);
        m_geometry_indices.swap(geometry_begin_indices);

        unsigned *geometries_list_ptr =
            data_layout->GetBlockPtr<unsigned>(shared_memory, SharedDataLayout::GEOMETRIES_LIST);
        typename ShM<unsigned, true>::vector geometry_list(
            geometries_list_ptr, data_layout->num_entries[SharedDataLayout::GEOMETRIES_LIST]);
        m_geometry_list.swap(geometry_list);
    }

  public:
    virtual ~SharedDataFacade() {}

    SharedDataFacade()
    {
        data_timestamp_ptr = (SharedDataTimestamp *)SharedMemoryFactory::Get(
                                 CURRENT_REGIONS, sizeof(SharedDataTimestamp), false, false)->Ptr();
        CURRENT_LAYOUT = LAYOUT_NONE;
        CURRENT_DATA = DATA_NONE;
        CURRENT_TIMESTAMP = 0;

        // load data
        CheckAndReloadFacade();
    }

    void CheckAndReloadFacade()
    {
        if (CURRENT_LAYOUT != data_timestamp_ptr->layout ||
            CURRENT_DATA != data_timestamp_ptr->data ||
            CURRENT_TIMESTAMP != data_timestamp_ptr->timestamp)
        {
            // release the previous shared memory segments
            SharedMemory::Remove(CURRENT_LAYOUT);
            SharedMemory::Remove(CURRENT_DATA);

            CURRENT_LAYOUT = data_timestamp_ptr->layout;
            CURRENT_DATA = data_timestamp_ptr->data;
            CURRENT_TIMESTAMP = data_timestamp_ptr->timestamp;

            m_layout_memory.reset(SharedMemoryFactory::Get(CURRENT_LAYOUT));

            data_layout = (SharedDataLayout *)(m_layout_memory->Ptr());

            m_large_memory.reset(SharedMemoryFactory::Get(CURRENT_DATA));
            shared_memory = (char *)(m_large_memory->Ptr());

            const char *file_index_ptr =
                data_layout->GetBlockPtr<char>(shared_memory, SharedDataLayout::FILE_INDEX_PATH);
            file_index_path = boost::filesystem::path(file_index_ptr);
            if (!boost::filesystem::exists(file_index_path))
            {
                SimpleLogger().Write(logDEBUG) << "Leaf file name " << file_index_path.string();
                throw osrm::exception("Could not load leaf index file."
                                      "Is any data loaded into shared memory?");
            }

            LoadGraph();
            LoadChecksum();
            LoadNodeAndEdgeInformation();
            LoadGeometries();
            LoadTimestamp();
            LoadViaNodeList();
            LoadNames();

            data_layout->PrintInformation();

            SimpleLogger().Write() << "number of geometries: " << m_coordinate_list->size();
            for (unsigned i = 0; i < m_coordinate_list->size(); ++i)
            {
                if (!GetCoordinateOfNode(i).is_valid())
                {
                    SimpleLogger().Write() << "coordinate " << i << " not valid";
                }
            }
        }
    }

    // search graph access
    unsigned GetNumberOfNodes() const override final { return m_query_graph->GetNumberOfNodes(); }

    unsigned GetNumberOfEdges() const override final { return m_query_graph->GetNumberOfEdges(); }

    unsigned GetOutDegree(const NodeID n) const override final
    {
        return m_query_graph->GetOutDegree(n);
    }

    NodeID GetTarget(const EdgeID e) const override final { return m_query_graph->GetTarget(e); }

    EdgeDataT &GetEdgeData(const EdgeID e) const override final
    {
        return m_query_graph->GetEdgeData(e);
    }

    EdgeID BeginEdges(const NodeID n) const override final { return m_query_graph->BeginEdges(n); }

    EdgeID EndEdges(const NodeID n) const override final { return m_query_graph->EndEdges(n); }

    EdgeRange GetAdjacentEdgeRange(const NodeID node) const override final
    {
        return m_query_graph->GetAdjacentEdgeRange(node);
    };

    // searches for a specific edge
    EdgeID FindEdge(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdge(from, to);
    }

    EdgeID FindEdgeInEitherDirection(const NodeID from, const NodeID to) const override final
    {
        return m_query_graph->FindEdgeInEitherDirection(from, to);
    }

    EdgeID
    FindEdgeIndicateIfReverse(const NodeID from, const NodeID to, bool &result) const override final
    {
        return m_query_graph->FindEdgeIndicateIfReverse(from, to, result);
    }

    // node and edge information access
    FixedPointCoordinate GetCoordinateOfNode(const NodeID id) const override final
    {
        return m_coordinate_list->at(id);
    };

    virtual bool EdgeIsCompressed(const unsigned id) const override final
    {
        return m_edge_is_compressed.at(id);
    }

    virtual void GetUncompressedGeometry(const unsigned id,
                                         std::vector<unsigned> &result_nodes) const override final
    {
        const unsigned begin = m_geometry_indices.at(id);
        const unsigned end = m_geometry_indices.at(id + 1);

        result_nodes.clear();
        result_nodes.insert(result_nodes.begin(), m_geometry_list.begin() + begin,
                            m_geometry_list.begin() + end);
    }

    virtual unsigned GetGeometryIndexForEdgeID(const unsigned id) const override final
    {
        return m_via_node_list.at(id);
    }

    TurnInstruction GetTurnInstructionForEdgeID(const unsigned id) const override final
    {
        return m_turn_instruction_list.at(id);
    }

    TravelMode GetTravelModeForEdgeID(const unsigned id) const override final
    {
        return m_travel_mode_list.at(id);
    }

    bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                            FixedPointCoordinate &result,
                                            const unsigned zoom_level = 18) override final
    {
        if (!m_static_rtree.get() || CURRENT_TIMESTAMP != m_static_rtree->first)
        {
            LoadRTree();
        }

        return m_static_rtree->second->LocateClosestEndPointForCoordinate(input_coordinate, result,
                                                                          zoom_level);
    }

    bool IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                                 PhantomNode &resulting_phantom_node) override final
    {
        std::vector<PhantomNode> resulting_phantom_node_vector;
        auto result = IncrementalFindPhantomNodeForCoordinate(input_coordinate,
                                                              resulting_phantom_node_vector, 1);
        if (result)
        {
            BOOST_ASSERT(!resulting_phantom_node_vector.empty());
            resulting_phantom_node = resulting_phantom_node_vector.front();
        }

        return result;
    }

    bool
    IncrementalFindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                            std::vector<PhantomNode> &resulting_phantom_node_vector,
                                            const unsigned number_of_results) override final
    {
        if (!m_static_rtree.get() || CURRENT_TIMESTAMP != m_static_rtree->first)
        {
            LoadRTree();
        }

        return m_static_rtree->second->IncrementalFindPhantomNodeForCoordinate(
            input_coordinate, resulting_phantom_node_vector, number_of_results);
    }

    bool IncrementalFindPhantomNodeForCoordinateWithMaxDistance(
        const FixedPointCoordinate &input_coordinate,
        std::vector<std::pair<PhantomNode, double>> &resulting_phantom_node_vector,
        const double max_distance,
        const unsigned min_number_of_phantom_nodes,
        const unsigned max_number_of_phantom_nodes) override final
    {
        if (!m_static_rtree.get() || CURRENT_TIMESTAMP != m_static_rtree->first)
        {
            LoadRTree();
        }

        return m_static_rtree->second->IncrementalFindPhantomNodeForCoordinateWithDistance(
            input_coordinate, resulting_phantom_node_vector, max_distance,
            min_number_of_phantom_nodes, max_number_of_phantom_nodes);
    }

    unsigned GetCheckSum() const override final { return m_check_sum; }

    unsigned GetNameIndexFromEdgeID(const unsigned id) const override final
    {
        return m_name_ID_list.at(id);
    };

    std::string get_name_for_id(const unsigned name_id) const override final
    {
        if (std::numeric_limits<unsigned>::max() == name_id)
        {
            return "";
        }
        auto range = m_name_table->GetRange(name_id);

        std::string result;
        result.reserve(range.size());
        if (range.begin() != range.end())
        {
            result.resize(range.back() - range.front() + 1);
            std::copy(m_names_char_list.begin() + range.front(),
                      m_names_char_list.begin() + range.back() + 1, result.begin());
        }
        return result;
    }

    std::string GetTimestamp() const override final { return m_timestamp; }
};

#endif // SHARED_DATAFACADE_HPP
