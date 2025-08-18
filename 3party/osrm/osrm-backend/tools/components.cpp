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

#include "../typedefs.h"
#include "../algorithms/tiny_components.hpp"
#include "../data_structures/coordinate_calculation.hpp"
#include "../data_structures/dynamic_graph.hpp"
#include "../data_structures/static_graph.hpp"
#include "../util/fingerprint.hpp"
#include "../util/graph_loader.hpp"
#include "../util/make_unique.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"

#include <boost/filesystem.hpp>

#if defined(__APPLE__) || defined(_WIN32)
#include <gdal.h>
#include <ogrsf_frmts.h>
#else
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#endif

#include <osrm/coordinate.hpp>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{

struct TarjanEdgeData
{
    TarjanEdgeData() : distance(INVALID_EDGE_WEIGHT), name_id(INVALID_NAMEID) {}
    TarjanEdgeData(unsigned distance, unsigned name_id) : distance(distance), name_id(name_id) {}
    unsigned distance;
    unsigned name_id;
};

using TarjanGraph = StaticGraph<TarjanEdgeData>;
using TarjanEdge = TarjanGraph::InputEdge;

void DeleteFileIfExists(const std::string &file_name)
{
    if (boost::filesystem::exists(file_name))
    {
        boost::filesystem::remove(file_name);
    }
}
}

int main(int argc, char *argv[])
{
    std::vector<QueryNode> coordinate_list;
    std::vector<TurnRestriction> restriction_list;
    std::vector<NodeID> bollard_node_list;
    std::vector<NodeID> traffic_lights_list;

    LogPolicy::GetInstance().Unmute();
    try
    {
        // enable logging
        if (argc < 3)
        {
            SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0]
                                             << " <osrm> <osrm.restrictions>";
            return -1;
        }

        SimpleLogger().Write() << "Using restrictions from file: " << argv[2];
        std::ifstream restriction_ifstream(argv[2], std::ios::binary);
        const FingerPrint fingerprint_orig;
        FingerPrint fingerprint_loaded;
        restriction_ifstream.read(reinterpret_cast<char *>(&fingerprint_loaded),
                                  sizeof(FingerPrint));

        // check fingerprint and warn if necessary
        if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
        {
            SimpleLogger().Write(logWARNING) << argv[2] << " was prepared with a different build. "
                                                           "Reprocess to get rid of this warning.";
        }

        if (!restriction_ifstream.good())
        {
            throw osrm::exception("Could not access <osrm-restrictions> files");
        }
        uint32_t usable_restrictions = 0;
        restriction_ifstream.read(reinterpret_cast<char *>(&usable_restrictions), sizeof(uint32_t));
        restriction_list.resize(usable_restrictions);

        // load restrictions
        if (usable_restrictions > 0)
        {
            restriction_ifstream.read(reinterpret_cast<char *>(&restriction_list[0]),
                                      usable_restrictions * sizeof(TurnRestriction));
        }
        restriction_ifstream.close();

        std::ifstream input_stream(argv[1], std::ifstream::in | std::ifstream::binary);
        if (!input_stream.is_open())
        {
            throw osrm::exception("Cannot open osrm file");
        }

        // load graph data
        std::vector<ImportEdge> edge_list;
        const NodeID number_of_nodes =
            readBinaryOSRMGraphFromStream(input_stream, edge_list, bollard_node_list,
                                          traffic_lights_list, &coordinate_list, restriction_list);
        input_stream.close();

        BOOST_ASSERT_MSG(restriction_list.size() == usable_restrictions,
                         "size of restriction_list changed");

        SimpleLogger().Write() << restriction_list.size() << " restrictions, "
                               << bollard_node_list.size() << " bollard nodes, "
                               << traffic_lights_list.size() << " traffic lights";

        traffic_lights_list.clear();
        traffic_lights_list.shrink_to_fit();

        // Building an node-based graph
        std::vector<TarjanEdge> graph_edge_list;
//        DeallocatingVector<TarjanEdge> graph_edge_list;
        for (const auto &input_edge : edge_list)
        {
            if (input_edge.source == input_edge.target)
            {
                continue;
            }

            if (input_edge.forward)
            {
                graph_edge_list.emplace_back(input_edge.source, input_edge.target,
                                             (std::max)(input_edge.weight, 1), input_edge.name_id);
            }
            if (input_edge.backward)
            {
                graph_edge_list.emplace_back(input_edge.target, input_edge.source,
                                             (std::max)(input_edge.weight, 1), input_edge.name_id);
            }
        }
        edge_list.clear();
        edge_list.shrink_to_fit();
        BOOST_ASSERT_MSG(0 == edge_list.size() && 0 == edge_list.capacity(),
                         "input edge vector not properly deallocated");

        tbb::parallel_sort(graph_edge_list.begin(), graph_edge_list.end());
        const auto graph = std::make_shared<TarjanGraph>(number_of_nodes, graph_edge_list);
        graph_edge_list.clear();
        graph_edge_list.shrink_to_fit();

        SimpleLogger().Write() << "Starting SCC graph traversal";

        RestrictionMap restriction_map(restriction_list);
        auto tarjan = osrm::make_unique<TarjanSCC<TarjanGraph>>(graph,
                                                                       restriction_map,
                                                                       bollard_node_list);
        tarjan->run();
        SimpleLogger().Write() << "identified: " << tarjan->get_number_of_components()
                               << " many components";
        SimpleLogger().Write() << "identified " << tarjan->get_size_one_count() << " size 1 SCCs";

        // output
        TIMER_START(SCC_RUN_SETUP);

        // remove files from previous run if exist
        DeleteFileIfExists("component.dbf");
        DeleteFileIfExists("component.shx");
        DeleteFileIfExists("component.shp");

        Percent percentage(graph->GetNumberOfNodes());

        OGRRegisterAll();

        const char *pszDriverName = "ESRI Shapefile";
        OGRSFDriver *poDriver =
            OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName);
        if (nullptr == poDriver)
        {
            throw osrm::exception("ESRI Shapefile driver not available");
        }
        OGRDataSource *poDS = poDriver->CreateDataSource("component.shp", nullptr);

        if (nullptr == poDS)
        {
            throw osrm::exception("Creation of output file failed");
        }

        OGRSpatialReference *poSRS = new OGRSpatialReference();
        poSRS->importFromEPSG(4326);

        OGRLayer *poLayer = poDS->CreateLayer("component", poSRS, wkbLineString, nullptr);

        if (nullptr == poLayer)
        {
            throw osrm::exception("Layer creation failed.");
        }
        TIMER_STOP(SCC_RUN_SETUP);
        SimpleLogger().Write() << "shapefile setup took " << TIMER_MSEC(SCC_RUN_SETUP) / 1000.
                               << "s";

        uint64_t total_network_length = 0;
        percentage.reinit(graph->GetNumberOfNodes());
        TIMER_START(SCC_OUTPUT);
        for (const NodeID source : osrm::irange(0u, graph->GetNumberOfNodes()))
        {
            percentage.printIncrement();
            for (const auto current_edge : graph->GetAdjacentEdgeRange(source))
            {
                const TarjanGraph::NodeIterator target = graph->GetTarget(current_edge);

                if (source < target || SPECIAL_EDGEID == graph->FindEdge(target, source))
                {
                    total_network_length +=
                        100 * coordinate_calculation::euclidean_distance(
                                  coordinate_list[source].lat, coordinate_list[source].lon,
                                  coordinate_list[target].lat, coordinate_list[target].lon);

                    BOOST_ASSERT(current_edge != SPECIAL_EDGEID);
                    BOOST_ASSERT(source != SPECIAL_NODEID);
                    BOOST_ASSERT(target != SPECIAL_NODEID);

                    const unsigned size_of_containing_component = std::min(
                        tarjan->get_component_size(source), tarjan->get_component_size(target));

                    // edges that end on bollard nodes may actually be in two distinct components
                    if (size_of_containing_component < 1000)
                    {
                        OGRLineString lineString;
                        lineString.addPoint(coordinate_list[source].lon / COORDINATE_PRECISION,
                                            coordinate_list[source].lat / COORDINATE_PRECISION);
                        lineString.addPoint(coordinate_list[target].lon / COORDINATE_PRECISION,
                                            coordinate_list[target].lat / COORDINATE_PRECISION);

                        OGRFeature *poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());

                        poFeature->SetGeometry(&lineString);
                        if (OGRERR_NONE != poLayer->CreateFeature(poFeature))
                        {
                            throw osrm::exception("Failed to create feature in shapefile.");
                        }
                        OGRFeature::DestroyFeature(poFeature);
                    }
                }
            }
        }
        OGRSpatialReference::DestroySpatialReference(poSRS);
        OGRDataSource::DestroyDataSource(poDS);
        TIMER_STOP(SCC_OUTPUT);
        SimpleLogger().Write() << "generating output took: " << TIMER_MSEC(SCC_OUTPUT) / 1000.
                               << "s";

        SimpleLogger().Write() << "total network distance: "
                               << static_cast<uint64_t>(total_network_length / 100 / 1000.)
                               << " km";

        SimpleLogger().Write() << "finished component analysis";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }
    return 0;
}
