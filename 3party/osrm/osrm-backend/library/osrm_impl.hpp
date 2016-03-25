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

#ifndef OSRM_IMPL_HPP
#define OSRM_IMPL_HPP

class BasePlugin;
struct RouteParameters;

#include "../data_structures/edge_based_node_data.hpp"
#include "../data_structures/query_edge.hpp"

#include <osrm/json_container.hpp>
#include <osrm/libosrm_config.hpp>

#include <memory>
#include <unordered_map>
#include <string>

struct SharedBarriers;
template <class EdgeDataT> class BaseDataFacade;

class OSRM_impl
{
  private:
    using PluginMap = std::unordered_map<std::string, BasePlugin *>;

  public:
    OSRM_impl(libosrm_config &lib_config);
    OSRM_impl(const OSRM_impl &) = delete;
    virtual ~OSRM_impl();
    int RunQuery(RouteParameters &route_parameters, osrm::json::Object &json_result);

  private:
    void RegisterPlugin(BasePlugin *plugin);
    PluginMap plugin_map;
    // will only be initialized if shared memory is used
    std::unique_ptr<SharedBarriers> barrier;
    // base class pointer to the objects
    BaseDataFacade<QueryEdge::EdgeData> *query_data_facade;

    // decrease number of concurrent queries
    void decrease_concurrent_query_count();
    // increase number of concurrent queries
    void increase_concurrent_query_count();

    osrm::NodeDataVectorT m_nodeData;
};

#endif // OSRM_IMPL_HPP
