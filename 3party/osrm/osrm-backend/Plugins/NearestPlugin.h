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

#ifndef NEAREST_PLUGIN_H
#define NEAREST_PLUGIN_H

#include "BasePlugin.h"
#include "../DataStructures/JSONContainer.h"
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/Range.h"

#include <string>

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */

template <class DataFacadeT> class NearestPlugin final : public BasePlugin
{
  public:
    explicit NearestPlugin(DataFacadeT *facade) : facade(facade), descriptor_string("nearest") {}

    const std::string GetDescriptor() const final { return descriptor_string; }

    void HandleRequest(const RouteParameters &route_parameters, http::Reply &reply) final
    {
        // check number of parameters
        if (route_parameters.coordinates.empty() || !route_parameters.coordinates.front().isValid())
        {
            reply = http::Reply::StockReply(http::Reply::badRequest);
            return;
        }
        auto number_of_results = static_cast<std::size_t>(route_parameters.num_results);
        std::vector<PhantomNode> phantom_node_vector;
        facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates.front(),
                                                        phantom_node_vector,
                                                        route_parameters.zoom_level,
                                                        static_cast<int>(number_of_results));

        JSON::Object json_result;
        if (phantom_node_vector.empty() || !phantom_node_vector.front().isValid())
        {
            json_result.values["status"] = 207;
        }
        else
        {
            reply.status = http::Reply::ok;
            json_result.values["status"] = 0;

            if (number_of_results > 1)
            {
                JSON::Array results;

                auto vector_length = phantom_node_vector.size();
                for (const auto i : osrm::irange<std::size_t>(0, std::min(number_of_results, vector_length)))
                {
                    JSON::Array json_coordinate;
                    JSON::Object result;
                    json_coordinate.values.push_back(phantom_node_vector.at(i).location.lat /
                                                     COORDINATE_PRECISION);
                    json_coordinate.values.push_back(phantom_node_vector.at(i).location.lon /
                                                     COORDINATE_PRECISION);
                    result.values["mapped coordinate"] = json_coordinate;
                    std::string temp_string;
                    facade->GetName(phantom_node_vector.front().name_id, temp_string);
                    result.values["name"] = temp_string;
                    results.values.push_back(result);
                }
                json_result.values["results"] = results;
            }
            else
            {
                JSON::Array json_coordinate;
                json_coordinate.values.push_back(phantom_node_vector.front().location.lat /
                                                 COORDINATE_PRECISION);
                json_coordinate.values.push_back(phantom_node_vector.front().location.lon /
                                                 COORDINATE_PRECISION);
                json_result.values["mapped_coordinate"] = json_coordinate;
                std::string temp_string;
                facade->GetName(phantom_node_vector.front().name_id, temp_string);
                json_result.values["name"] = temp_string;
            }
        }
        JSON::render(reply.content, json_result);
    }

  private:
    DataFacadeT *facade;
    std::string descriptor_string;
};

#endif /* NEAREST_PLUGIN_H */
