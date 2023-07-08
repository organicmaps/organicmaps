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

#ifndef NEAREST_HPP
#define NEAREST_HPP

#include "plugin_base.hpp"

#include "../data_structures/phantom_node.hpp"
#include "../util/integer_range.hpp"
#include "../util/json_renderer.hpp"

#include <osrm/json_container.hpp>

#include <string>

/*
 * This Plugin locates the nearest point on a street in the road network for a given coordinate.
 */

template <class DataFacadeT> class NearestPlugin final : public BasePlugin
{
  public:
    explicit NearestPlugin(DataFacadeT *facade) : facade(facade), descriptor_string("nearest") {}

    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        // check number of parameters
        if (route_parameters.coordinates.empty() ||
            !route_parameters.coordinates.front().is_valid())
        {
            return 400;
        }
        auto number_of_results = static_cast<std::size_t>(route_parameters.num_results);
        std::vector<PhantomNode> phantom_node_vector;
        facade->IncrementalFindPhantomNodeForCoordinate(route_parameters.coordinates.front(),
                                                        phantom_node_vector,
                                                        static_cast<int>(number_of_results));

        if (phantom_node_vector.empty() || !phantom_node_vector.front().is_valid())
        {
            json_result.values["status"] = 207;
        }
        else
        {
            // reply.status = http::Reply::ok;
            json_result.values["status"] = 0;

            if (number_of_results > 1)
            {
                osrm::json::Array results;

                auto vector_length = phantom_node_vector.size();
                for (const auto i :
                     osrm::irange<std::size_t>(0, std::min(number_of_results, vector_length)))
                {
                    osrm::json::Array json_coordinate;
                    osrm::json::Object result;
                    json_coordinate.values.push_back(phantom_node_vector.at(i).location.lat /
                                                     COORDINATE_PRECISION);
                    json_coordinate.values.push_back(phantom_node_vector.at(i).location.lon /
                                                     COORDINATE_PRECISION);
                    result.values["mapped coordinate"] = json_coordinate;
                    result.values["name"] =
                        facade->get_name_for_id(phantom_node_vector.at(i).name_id);
                    results.values.push_back(result);
                }
                json_result.values["results"] = results;
            }
            else
            {
                osrm::json::Array json_coordinate;
                json_coordinate.values.push_back(phantom_node_vector.front().location.lat /
                                                 COORDINATE_PRECISION);
                json_coordinate.values.push_back(phantom_node_vector.front().location.lon /
                                                 COORDINATE_PRECISION);
                json_result.values["mapped_coordinate"] = json_coordinate;
                json_result.values["name"] =
                    facade->get_name_for_id(phantom_node_vector.front().name_id);
            }
        }
        return 200;
    }

  private:
    DataFacadeT *facade;
    std::string descriptor_string;
};

#endif /* NEAREST_HPP */
