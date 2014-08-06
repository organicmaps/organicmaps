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
#include "../DataStructures/DeallocatingVector.h"
#include "../DataStructures/Percent.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/Range.h"
#include "../DataStructures/StaticGraph.h"
#include "../Util/GraphLoader.h"
#include "../Util/SimpleLogger.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>

#include <memory>
#include <vector>

typedef QueryEdge::EdgeData EdgeData;
typedef StaticGraph<EdgeData> QueryGraph;

int main(int argc, char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    if (argc != 2)
    {
        SimpleLogger().Write(logWARNING) << "usage: " << argv[0] << " <file.hsgr>";
        return 1;
    }

    try
    {
        boost::filesystem::path hsgr_path(argv[1]);

        std::vector<QueryGraph::NodeArrayEntry> node_list;
        std::vector<QueryGraph::EdgeArrayEntry> edge_list;

        SimpleLogger().Write() << "loading graph from " << hsgr_path.string();

        unsigned m_check_sum = 0;
        unsigned m_number_of_nodes = readHSGRFromStream(hsgr_path, node_list, edge_list, &m_check_sum);
        SimpleLogger().Write() << "announced " << m_number_of_nodes
                               << " nodes, checksum: " << m_check_sum;
        BOOST_ASSERT_MSG(0 != node_list.size(), "node list empty");
        // BOOST_ASSERT_MSG(0 != edge_list.size(), "edge list empty");
        SimpleLogger().Write() << "loaded " << node_list.size() << " nodes and " << edge_list.size()
                               << " edges";
        std::shared_ptr<QueryGraph> m_query_graph = std::make_shared<QueryGraph>(node_list, edge_list);

        BOOST_ASSERT_MSG(0 == node_list.size(), "node list not flushed");
        BOOST_ASSERT_MSG(0 == edge_list.size(), "edge list not flushed");

        Percent p(m_query_graph->GetNumberOfNodes());
        for (const auto u : osrm::irange(0u, m_query_graph->GetNumberOfNodes()))
        {
            for (const auto eid : m_query_graph->GetAdjacentEdgeRange(u))
            {
                const EdgeData &data = m_query_graph->GetEdgeData(eid);
                if (!data.shortcut)
                {
                    continue;
                }
                const unsigned v = m_query_graph->GetTarget(eid);
                const EdgeID first_edge_id = m_query_graph->FindEdgeInEitherDirection(u, data.id);
                if (SPECIAL_EDGEID == first_edge_id)
                {
                    SimpleLogger().Write(logWARNING) << "cannot find first segment of edge (" << u
                                                     << "," << data.id << "," << v << "), eid: " << eid;
                    BOOST_ASSERT(false);
                    return 1;
                }
                const EdgeID second_edge_id = m_query_graph->FindEdgeInEitherDirection(data.id, v);
                if (SPECIAL_EDGEID == second_edge_id)
                {
                    SimpleLogger().Write(logWARNING) << "cannot find second segment of edge (" << u
                                                     << "," << data.id << "," << v << "), eid: " << eid;
                    BOOST_ASSERT(false);
                    return 1;
                }
            }
            p.printIncrement();
        }
        m_query_graph.reset();
        SimpleLogger().Write() << "Data file " << argv[0] << " appears to be OK";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }
    return 0;
}
