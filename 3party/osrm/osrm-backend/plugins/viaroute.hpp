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

#ifndef VIA_ROUTE_HPP
#define VIA_ROUTE_HPP

#include "plugin_base.hpp"

#include "../algorithms/object_encoder.hpp"
#include "../data_structures/search_engine.hpp"
#include "../descriptors/descriptor_base.hpp"
#include "../descriptors/gpx_descriptor.hpp"
#include "../descriptors/json_descriptor.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"
#include "../util/make_unique.hpp"
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>

#include <cstdlib>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

template <class DataFacadeT> class ViaRoutePlugin final : public BasePlugin
{
  private:
    DescriptorTable descriptor_table;
    std::string descriptor_string;
    std::unique_ptr<SearchEngine<DataFacadeT>> search_engine_ptr;
    DataFacadeT *facade;

  public:
    explicit ViaRoutePlugin(DataFacadeT *facade) : descriptor_string("viaroute"), facade(facade)
    {
        search_engine_ptr = osrm::make_unique<SearchEngine<DataFacadeT>>(facade);

        descriptor_table.emplace("json", 0);
        descriptor_table.emplace("gpx", 1);
        // descriptor_table.emplace("geojson", 2);
    }

    virtual ~ViaRoutePlugin() {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        if (!check_all_coordinates(route_parameters.coordinates))
        {
            return 400;
        }

        std::vector<phantom_node_pair> phantom_node_pair_list(route_parameters.coordinates.size());
        const bool checksum_OK = (route_parameters.check_sum == facade->GetCheckSum());

        for (const auto i : osrm::irange<std::size_t>(0, route_parameters.coordinates.size()))
        {
            if (checksum_OK && i < route_parameters.hints.size() &&
                !route_parameters.hints[i].empty())
            {
                ObjectEncoder::DecodeFromBase64(route_parameters.hints[i],
                                                phantom_node_pair_list[i]);
                if (phantom_node_pair_list[i].first.is_valid(facade->GetNumberOfNodes()))
                {
                    continue;
                }
            }
            std::vector<PhantomNode> phantom_node_vector;
            if (facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates[i],
                                                                phantom_node_vector, 1))
            {
                BOOST_ASSERT(!phantom_node_vector.empty());
                phantom_node_pair_list[i].first = phantom_node_vector.front();
                if (phantom_node_vector.size() > 1)
                {
                    phantom_node_pair_list[i].second = phantom_node_vector.back();
                }
            }
        }

        auto check_component_id_is_tiny = [](const phantom_node_pair &phantom_pair)
        {
            return phantom_pair.first.component_id != 0;
        };

        const bool every_phantom_is_in_tiny_cc =
            std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                        check_component_id_is_tiny);

        // are all phantoms from a tiny cc?
        const auto component_id = phantom_node_pair_list.front().first.component_id;

        auto check_component_id_is_equal = [component_id](const phantom_node_pair &phantom_pair)
        {
            return component_id == phantom_pair.first.component_id;
        };

        const bool every_phantom_has_equal_id =
            std::all_of(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                        check_component_id_is_equal);

        auto swap_phantom_from_big_cc_into_front = [](phantom_node_pair &phantom_pair)
        {
            if (0 != phantom_pair.first.component_id)
            {
                using namespace std;
                swap(phantom_pair.first, phantom_pair.second);
            }
        };

        // this case is true if we take phantoms from the big CC
        if (!every_phantom_is_in_tiny_cc || !every_phantom_has_equal_id)
        {
            std::for_each(std::begin(phantom_node_pair_list), std::end(phantom_node_pair_list),
                          swap_phantom_from_big_cc_into_front);
        }

        InternalRouteResult raw_route;
        auto build_phantom_pairs =
            [&raw_route](const phantom_node_pair &first_pair, const phantom_node_pair &second_pair)
        {
            raw_route.segment_end_coordinates.emplace_back(
                PhantomNodes{first_pair.first, second_pair.first});
        };
        osrm::for_each_pair(phantom_node_pair_list, build_phantom_pairs);

        if (route_parameters.alternate_route && 1 == raw_route.segment_end_coordinates.size())
        {
            search_engine_ptr->alternative_path(raw_route.segment_end_coordinates.front(),
                                                raw_route);
        }
        else
        {
            search_engine_ptr->shortest_path(raw_route.segment_end_coordinates,
                                             route_parameters.uturns, raw_route);
        }

        if (INVALID_EDGE_WEIGHT == raw_route.shortest_path_length)
        {
            SimpleLogger().Write(logDEBUG) << "Error occurred, single path not found";
        }

        std::unique_ptr<BaseDescriptor<DataFacadeT>> descriptor;
        switch (descriptor_table.get_id(route_parameters.output_format))
        {
        case 1:
            descriptor = osrm::make_unique<GPXDescriptor<DataFacadeT>>(facade);
            break;
        // case 2:
        //      descriptor = osrm::make_unique<GEOJSONDescriptor<DataFacadeT>>();
        //      break;
        default:
            descriptor = osrm::make_unique<JSONDescriptor<DataFacadeT>>(facade);
            break;
        }

        descriptor->SetConfig(route_parameters);
        descriptor->Run(raw_route, json_result);
        return 200;
    }
};

#endif // VIA_ROUTE_HPP
