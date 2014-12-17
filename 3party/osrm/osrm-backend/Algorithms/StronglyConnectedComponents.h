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

#ifndef STRONGLYCONNECTEDCOMPONENTS_H_
#define STRONGLYCONNECTEDCOMPONENTS_H_

#include "../typedefs.h"
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/DynamicGraph.h"
#include "../DataStructures/ImportEdge.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/Range.h"
#include "../DataStructures/Restriction.h"
#include "../DataStructures/TurnInstructions.h"

#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../Util/StdHashExtensions.h"
#include "../Util/TimingUtil.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>

#include <tbb/parallel_sort.h>

#ifdef __APPLE__
#include <gdal.h>
#include <ogrsf_frmts.h>
#else
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#endif

#include <cstdint>

#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TarjanSCC
{
  private:
    struct TarjanNode
    {
        TarjanNode() : index(SPECIAL_NODEID), low_link(SPECIAL_NODEID), on_stack(false) {}
        unsigned index;
        unsigned low_link;
        bool on_stack;
    };

    struct TarjanEdgeData
    {
        TarjanEdgeData() : distance(INVALID_EDGE_WEIGHT), name_id(INVALID_NAMEID) {}
        TarjanEdgeData(int distance, unsigned name_id) : distance(distance), name_id(name_id) {}
        int distance;
        unsigned name_id;
    };

    struct TarjanStackFrame
    {
        explicit TarjanStackFrame(NodeID v, NodeID parent) : v(v), parent(parent) {}
        NodeID v;
        NodeID parent;
    };

    using TarjanDynamicGraph = DynamicGraph<TarjanEdgeData>;
    using TarjanEdge = TarjanDynamicGraph::InputEdge;
    using RestrictionSource = std::pair<NodeID, NodeID>;
    using RestrictionTarget = std::pair<NodeID, bool>;
    using EmanatingRestrictionsVector = std::vector<RestrictionTarget>;
    using RestrictionMap = std::unordered_map<RestrictionSource, unsigned>;

    std::vector<NodeInfo> m_coordinate_list;
    std::vector<EmanatingRestrictionsVector> m_restriction_bucket_list;
    std::shared_ptr<TarjanDynamicGraph> m_node_based_graph;
    std::unordered_set<NodeID> barrier_node_list;
    std::unordered_set<NodeID> traffic_light_list;
    unsigned m_restriction_counter;
    RestrictionMap m_restriction_map;

  public:
    TarjanSCC(int number_of_nodes,
              std::vector<NodeBasedEdge> &input_edges,
              std::vector<NodeID> &bn,
              std::vector<NodeID> &tl,
              std::vector<TurnRestriction> &irs,
              std::vector<NodeInfo> &nI)
        : m_coordinate_list(nI), m_restriction_counter(irs.size())
    {
        TIMER_START(SCC_LOAD);
        for (const TurnRestriction &restriction : irs)
        {
            std::pair<NodeID, NodeID> restriction_source = {restriction.fromNode,
                                                            restriction.viaNode};
            unsigned index = 0;
            const auto restriction_iterator = m_restriction_map.find(restriction_source);
            if (restriction_iterator == m_restriction_map.end())
            {
                index = m_restriction_bucket_list.size();
                m_restriction_bucket_list.resize(index + 1);
                m_restriction_map.emplace(restriction_source, index);
            }
            else
            {
                index = restriction_iterator->second;
                // Map already contains an is_only_*-restriction
                if (m_restriction_bucket_list.at(index).begin()->second)
                {
                    continue;
                }
                else if (restriction.flags.isOnly)
                {
                    // We are going to insert an is_only_*-restriction. There can be only one.
                    m_restriction_bucket_list.at(index).clear();
                }
            }

            m_restriction_bucket_list.at(index)
                .emplace_back(restriction.toNode, restriction.flags.isOnly);
        }

        barrier_node_list.insert(bn.begin(), bn.end());
        traffic_light_list.insert(tl.begin(), tl.end());

        DeallocatingVector<TarjanEdge> edge_list;
        for (const NodeBasedEdge &input_edge : input_edges)
        {
            if (input_edge.source == input_edge.target)
            {
                continue;
            }

            if (input_edge.forward)
            {
                edge_list.emplace_back(input_edge.source,
                                       input_edge.target,
                                       (std::max)((int)input_edge.weight, 1),
                                       input_edge.name_id);
            }
            if (input_edge.backward)
            {
                edge_list.emplace_back(input_edge.target,
                                       input_edge.source,
                                       (std::max)((int)input_edge.weight, 1),
                                       input_edge.name_id);
            }
        }
        input_edges.clear();
        input_edges.shrink_to_fit();
        BOOST_ASSERT_MSG(0 == input_edges.size() && 0 == input_edges.capacity(),
                         "input edge vector not properly deallocated");

        tbb::parallel_sort(edge_list.begin(), edge_list.end());
        m_node_based_graph = std::make_shared<TarjanDynamicGraph>(number_of_nodes, edge_list);
        TIMER_STOP(SCC_LOAD);
        SimpleLogger().Write() << "Loading data into SCC took " << TIMER_MSEC(SCC_LOAD)/1000. << "s";
    }

    ~TarjanSCC() { m_node_based_graph.reset(); }

    void Run()
    {
        TIMER_START(SCC_RUN_SETUP);
        // remove files from previous run if exist
        DeleteFileIfExists("component.dbf");
        DeleteFileIfExists("component.shx");
        DeleteFileIfExists("component.shp");

        Percent p(m_node_based_graph->GetNumberOfNodes());

        OGRRegisterAll();

        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver *poDriver =
            OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName);
        if (nullptr == poDriver)
        {
            throw OSRMException("ESRI Shapefile driver not available");
        }
        OGRDataSource *poDS = poDriver->CreateDataSource("component.shp", nullptr);

        if (nullptr == poDS)
        {
            throw OSRMException("Creation of output file failed");
        }

        OGRSpatialReference *poSRS = new OGRSpatialReference();
        poSRS->importFromEPSG(4326);

        OGRLayer *poLayer = poDS->CreateLayer("component", poSRS, wkbLineString, nullptr);

        if (nullptr == poLayer)
        {
            throw OSRMException("Layer creation failed.");
        }
        TIMER_STOP(SCC_RUN_SETUP);
        SimpleLogger().Write() << "shapefile setup took " << TIMER_MSEC(SCC_RUN_SETUP)/1000. << "s";

        TIMER_START(SCC_RUN);
        // The following is a hack to distinguish between stuff that happens
        // before the recursive call and stuff that happens after
        std::stack<TarjanStackFrame> recursion_stack;
        // true = stuff before, false = stuff after call
        std::stack<NodeID> tarjan_stack;
        std::vector<unsigned> components_index(m_node_based_graph->GetNumberOfNodes(),
                                               SPECIAL_NODEID);
        std::vector<NodeID> component_size_vector;
        std::vector<TarjanNode> tarjan_node_list(m_node_based_graph->GetNumberOfNodes());
        unsigned component_index = 0, size_of_current_component = 0;
        int index = 0;
        const NodeID last_node = m_node_based_graph->GetNumberOfNodes();
        std::vector<bool> processing_node_before_recursion(m_node_based_graph->GetNumberOfNodes(), true);
        for(const NodeID node : osrm::irange(0u, last_node))
        {
            if (SPECIAL_NODEID == components_index[node])
            {
                recursion_stack.emplace(TarjanStackFrame(node, node));
            }

            while (!recursion_stack.empty())
            {
                TarjanStackFrame currentFrame = recursion_stack.top();
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

                    // Traverse outgoing edges
                    for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(v))
                    {
                        const TarjanDynamicGraph::NodeIterator vprime =
                            m_node_based_graph->GetTarget(current_edge);
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
        SimpleLogger().Write() << "SCC run took: " << TIMER_MSEC(SCC_RUN)/1000. << "s";
        SimpleLogger().Write() << "identified: " << component_size_vector.size()
                               << " many components, marking small components";

        TIMER_START(SCC_OUTPUT);

        const unsigned size_one_counter = std::count_if(component_size_vector.begin(),
                                                        component_size_vector.end(),
                                                        [](unsigned value)
                                                        {
            return 1 == value;
        });

        SimpleLogger().Write() << "identified " << size_one_counter << " SCCs of size 1";

        uint64_t total_network_distance = 0;
        p.reinit(m_node_based_graph->GetNumberOfNodes());
        // const NodeID last_u_node = m_node_based_graph->GetNumberOfNodes();
        for (const NodeID source : osrm::irange(0u, last_node))
        {
            p.printIncrement();
            for (const auto current_edge : m_node_based_graph->GetAdjacentEdgeRange(source))
            {
                const TarjanDynamicGraph::NodeIterator target =
                    m_node_based_graph->GetTarget(current_edge);

                if (source < target ||
                    m_node_based_graph->EndEdges(target) ==
                        m_node_based_graph->FindEdge(target, source))
                {
                    total_network_distance +=
                        100 * FixedPointCoordinate::ApproximateEuclideanDistance(
                                  m_coordinate_list[source].lat,
                                  m_coordinate_list[source].lon,
                                  m_coordinate_list[target].lat,
                                  m_coordinate_list[target].lon);

                    BOOST_ASSERT(current_edge != SPECIAL_EDGEID);
                    BOOST_ASSERT(source != SPECIAL_NODEID);
                    BOOST_ASSERT(target != SPECIAL_NODEID);

                    const unsigned size_of_containing_component =
                        std::min(component_size_vector[components_index[source]],
                                 component_size_vector[components_index[target]]);

                    // edges that end on bollard nodes may actually be in two distinct components
                    if (size_of_containing_component < 10)
                    {
                        OGRLineString lineString;
                        lineString.addPoint(m_coordinate_list[source].lon / COORDINATE_PRECISION,
                                            m_coordinate_list[source].lat / COORDINATE_PRECISION);
                        lineString.addPoint(m_coordinate_list[target].lon / COORDINATE_PRECISION,
                                            m_coordinate_list[target].lat / COORDINATE_PRECISION);

                        OGRFeature *poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());

                        poFeature->SetGeometry(&lineString);
                        if (OGRERR_NONE != poLayer->CreateFeature(poFeature))
                        {
                            throw OSRMException("Failed to create feature in shapefile.");
                        }
                        OGRFeature::DestroyFeature(poFeature);
                    }
                }
            }
        }
        OGRDataSource::DestroyDataSource(poDS);
        component_size_vector.clear();
        component_size_vector.shrink_to_fit();
        BOOST_ASSERT_MSG(0 == component_size_vector.size() && 0 == component_size_vector.capacity(),
                         "component_size_vector not properly deallocated");

        components_index.clear();
        components_index.shrink_to_fit();
        BOOST_ASSERT_MSG(0 == components_index.size() && 0 == components_index.capacity(),
                         "components_index not properly deallocated");
        TIMER_STOP(SCC_OUTPUT);
        SimpleLogger().Write() << "generating output took: " << TIMER_MSEC(SCC_OUTPUT)/1000. << "s";

        SimpleLogger().Write() << "total network distance: "
                               << (uint64_t)total_network_distance / 100 / 1000. << " km";
    }

  private:
    unsigned CheckForEmanatingIsOnlyTurn(const NodeID u, const NodeID v) const
    {
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        const auto restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const RestrictionSource &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (restriction_target.second)
                {
                    return restriction_target.first;
                }
            }
        }
        return SPECIAL_NODEID;
    }

    bool CheckIfTurnIsRestricted(const NodeID u, const NodeID v, const NodeID w) const
    {
        // only add an edge if turn is not a U-turn except it is the end of dead-end street.
        std::pair<NodeID, NodeID> restriction_source = {u, v};
        const auto restriction_iterator = m_restriction_map.find(restriction_source);
        if (restriction_iterator != m_restriction_map.end())
        {
            const unsigned index = restriction_iterator->second;
            for (const RestrictionTarget &restriction_target : m_restriction_bucket_list.at(index))
            {
                if (w == restriction_target.first)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void DeleteFileIfExists(const std::string &file_name) const
    {
        if (boost::filesystem::exists(file_name))
        {
            boost::filesystem::remove(file_name);
        }
    }
};

#endif /* STRONGLYCONNECTEDCOMPONENTS_H_ */
