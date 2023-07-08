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

#ifndef LOCATE_HPP
#define LOCATE_HPP

#include "plugin_base.hpp"

#include "../util/json_renderer.hpp"
#include "../util/string_util.hpp"

#include <osrm/json_container.hpp>

#include <string>

// locates the nearest node in the road network for a given coordinate.
template <class DataFacadeT> class LocatePlugin final : public BasePlugin
{
  public:
    explicit LocatePlugin(DataFacadeT *facade) : descriptor_string("locate"), facade(facade) {}
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

        FixedPointCoordinate result;
        if (!facade->LocateClosestEndPointForCoordinate(route_parameters.coordinates.front(),
                                                        result))
        {
            json_result.values["status"] = 207;
        }
        else
        {
            json_result.values["status"] = 0;
            osrm::json::Array json_coordinate;
            json_coordinate.values.push_back(result.lat / COORDINATE_PRECISION);
            json_coordinate.values.push_back(result.lon / COORDINATE_PRECISION);
            json_result.values["mapped_coordinate"] = json_coordinate;
        }
        return 200;
    }

  private:
    std::string descriptor_string;
    DataFacadeT *facade;
};

#endif /* LOCATE_HPP */
