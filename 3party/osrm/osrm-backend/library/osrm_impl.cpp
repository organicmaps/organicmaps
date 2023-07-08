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

namespace boost
{
namespace interprocess
{
class named_mutex;
}
}

#include "osrm_impl.hpp"
#include "osrm.hpp"

// Our plugin is first, because it has naming conflicts with osrm part.
#include "../plugins/MapsMePlugin.hpp"
#include "../plugins/WayIdPlugin.hpp"
#include "../plugins/distance_table.hpp"
#include "../plugins/hello_world.hpp"
#include "../plugins/locate.hpp"
#include "../plugins/nearest.hpp"
#include "../plugins/timestamp.hpp"
#include "../plugins/viaroute.hpp"
#include "../plugins/match.hpp"
#include "../server/data_structures/datafacade_base.hpp"
#include "../server/data_structures/internal_datafacade.hpp"
#include "../server/data_structures/shared_barriers.hpp"
#include "../server/data_structures/shared_datafacade.hpp"
#include "../util/make_unique.hpp"
#include "../util/routed_options.hpp"
#include "../util/simple_logger.hpp"

#include <boost/assert.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <osrm/route_parameters.hpp>

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

OSRM_impl::OSRM_impl(libosrm_config &lib_config)
{
    if (lib_config.use_shared_memory)
    {
        barrier = osrm::make_unique<SharedBarriers>();
        query_data_facade = new SharedDataFacade<QueryEdge::EdgeData>();
    }
    else
    {
        // populate base path
        populate_base_path(lib_config.server_paths);
        query_data_facade = new InternalDataFacade<QueryEdge::EdgeData>(lib_config.server_paths);
    }

    if (!osrm::LoadNodeDataFromFile(lib_config.server_paths["enodesdata"].string(), m_nodeData))
    {
      SimpleLogger().Write(logDEBUG) << "Can't load node data";
      return;
    }

    // The following plugins handle all requests.
    RegisterPlugin(new DistanceTablePlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_distance_table));
    RegisterPlugin(new HelloWorldPlugin());
    RegisterPlugin(new LocatePlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new NearestPlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new MapMatchingPlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.max_locations_map_matching));
    RegisterPlugin(new TimestampPlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new ViaRoutePlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade));
    RegisterPlugin(new MapsMePlugin<BaseDataFacade<QueryEdge::EdgeData>>(
        query_data_facade, lib_config.server_paths["borders"].string(), m_nodeData));
    RegisterPlugin(new WayIdPlugin<BaseDataFacade<QueryEdge::EdgeData>>(query_data_facade, m_nodeData));
}

OSRM_impl::~OSRM_impl()
{
    delete query_data_facade;
    for (PluginMap::value_type &plugin_pointer : plugin_map)
    {
        delete plugin_pointer.second;
    }
}

void OSRM_impl::RegisterPlugin(BasePlugin *plugin)
{
    SimpleLogger().Write() << "loaded plugin: " << plugin->GetDescriptor();
    if (plugin_map.find(plugin->GetDescriptor()) != plugin_map.end())
    {
        delete plugin_map.find(plugin->GetDescriptor())->second;
    }
    plugin_map.emplace(plugin->GetDescriptor(), plugin);
}

int OSRM_impl::RunQuery(RouteParameters &route_parameters, osrm::json::Object &json_result)
{
    const auto &plugin_iterator = plugin_map.find(route_parameters.service);

    if (plugin_map.end() == plugin_iterator)
    {
        return 400;
    }

    increase_concurrent_query_count();
    plugin_iterator->second->HandleRequest(route_parameters, json_result);
    decrease_concurrent_query_count();
    return 200;
}

// decrease number of concurrent queries
void OSRM_impl::decrease_concurrent_query_count()
{
    if (!barrier)
    {
        return;
    }
    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier->query_mutex);

    // decrement query count
    --(barrier->number_of_queries);
    BOOST_ASSERT_MSG(0 <= barrier->number_of_queries, "invalid number of queries");

    // notify all processes that were waiting for this condition
    if (0 == barrier->number_of_queries)
    {
        barrier->no_running_queries_condition.notify_all();
    }
}

// increase number of concurrent queries
void OSRM_impl::increase_concurrent_query_count()
{
    if (!barrier)
    {
        return;
    }

    // lock update pending
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> pending_lock(
        barrier->pending_update_mutex);

    // lock query
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> query_lock(
        barrier->query_mutex);

    // unlock update pending
    pending_lock.unlock();

    // increment query count
    ++(barrier->number_of_queries);

    (static_cast<SharedDataFacade<QueryEdge::EdgeData> *>(query_data_facade))
        ->CheckAndReloadFacade();
}

// proxy code for compilation firewall
OSRM::OSRM(libosrm_config &lib_config) : OSRM_pimpl_(osrm::make_unique<OSRM_impl>(lib_config)) {}

OSRM::~OSRM() { OSRM_pimpl_.reset(); }

int OSRM::RunQuery(RouteParameters &route_parameters, osrm::json::Object &json_result)
{
    return OSRM_pimpl_->RunQuery(route_parameters, json_result);
}
