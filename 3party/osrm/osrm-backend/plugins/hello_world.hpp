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

#ifndef HELLO_WORLD_HPP
#define HELLO_WORLD_HPP

#include "plugin_base.hpp"

#include "../util/cast.hpp"
#include "../util/json_renderer.hpp"

#include <osrm/json_container.hpp>

#include <string>

class HelloWorldPlugin final : public BasePlugin
{
  private:
    std::string temp_string;

  public:
    HelloWorldPlugin() : descriptor_string("hello") {}
    virtual ~HelloWorldPlugin() {}
    const std::string GetDescriptor() const override final { return descriptor_string; }

    int HandleRequest(const RouteParameters &routeParameters,
                      osrm::json::Object &json_result) override final
    {
        std::string temp_string;
        json_result.values["title"] = "Hello World";

        temp_string = cast::integral_to_string(routeParameters.zoom_level);
        json_result.values["zoom_level"] = temp_string;

        temp_string = cast::integral_to_string(routeParameters.check_sum);
        json_result.values["check_sum"] = temp_string;
        json_result.values["instructions"] = (routeParameters.print_instructions ? "yes" : "no");
        json_result.values["geometry"] = (routeParameters.geometry ? "yes" : "no");
        json_result.values["compression"] = (routeParameters.compression ? "yes" : "no");
        json_result.values["output_format"] =
            (!routeParameters.output_format.empty() ? "yes" : "no");

        json_result.values["jsonp_parameter"] =
            (!routeParameters.jsonp_parameter.empty() ? "yes" : "no");
        json_result.values["language"] = (!routeParameters.language.empty() ? "yes" : "no");

        temp_string = cast::integral_to_string(routeParameters.coordinates.size());
        json_result.values["location_count"] = temp_string;

        osrm::json::Array json_locations;
        unsigned counter = 0;
        for (const FixedPointCoordinate &coordinate : routeParameters.coordinates)
        {
            osrm::json::Object json_location;
            osrm::json::Array json_coordinates;

            json_coordinates.values.push_back(
                static_cast<double>(coordinate.lat / COORDINATE_PRECISION));
            json_coordinates.values.push_back(
                static_cast<double>(coordinate.lon / COORDINATE_PRECISION));
            json_location.values[cast::integral_to_string(counter)] = json_coordinates;
            json_locations.values.push_back(json_location);
            ++counter;
        }
        json_result.values["locations"] = json_locations;
        json_result.values["hint_count"] = routeParameters.hints.size();

        osrm::json::Array json_hints;
        counter = 0;
        for (const std::string &current_hint : routeParameters.hints)
        {
            json_hints.values.push_back(current_hint);
            ++counter;
        }
        json_result.values["hints"] = json_hints;
        return 200;
    }

  private:
    std::string descriptor_string;
};

#endif // HELLO_WORLD_HPP
