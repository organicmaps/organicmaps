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

#ifndef HELLO_WORLD_PLUGIN_H
#define HELLO_WORLD_PLUGIN_H

#include "BasePlugin.h"
#include "../DataStructures/JSONContainer.h"
#include "../Util/StringUtil.h"

#include <string>

class HelloWorldPlugin : public BasePlugin
{
  private:
    std::string temp_string;

  public:
    HelloWorldPlugin() : descriptor_string("hello") {}
    virtual ~HelloWorldPlugin() {}
    const std::string GetDescriptor() const { return descriptor_string; }

    void HandleRequest(const RouteParameters &routeParameters, http::Reply &reply)
    {
        reply.status = http::Reply::ok;

        JSON::Object json_result;
        std::string temp_string;
        json_result.values["title"] = "Hello World";

        temp_string = IntToString(routeParameters.zoom_level);
        json_result.values["zoom_level"] = temp_string;

        temp_string = UintToString(routeParameters.check_sum);
        json_result.values["check_sum"] = temp_string;
        json_result.values["instructions"] = (routeParameters.print_instructions ? "yes" : "no");
        json_result.values["geometry"] = (routeParameters.geometry ? "yes" : "no");
        json_result.values["compression"] = (routeParameters.compression ? "yes" : "no");
        json_result.values["output_format"] = (!routeParameters.output_format.empty() ? "yes" : "no");

        json_result.values["jsonp_parameter"] = (!routeParameters.jsonp_parameter.empty() ? "yes" : "no");
        json_result.values["language"] = (!routeParameters.language.empty() ? "yes" : "no");

        temp_string = UintToString(static_cast<unsigned>(routeParameters.coordinates.size()));
        json_result.values["location_count"] = temp_string;

        JSON::Array json_locations;
        unsigned counter = 0;
        for (const FixedPointCoordinate &coordinate : routeParameters.coordinates)
        {
            JSON::Object json_location;
            JSON::Array json_coordinates;

            json_coordinates.values.push_back(coordinate.lat / COORDINATE_PRECISION);
            json_coordinates.values.push_back(coordinate.lon / COORDINATE_PRECISION);
            json_location.values[UintToString(counter)] = json_coordinates;
            json_locations.values.push_back(json_location);
            ++counter;
        }
        json_result.values["locations"] = json_locations;
        json_result.values["hint_count"] = routeParameters.hints.size();

        JSON::Array json_hints;
        counter = 0;
        for (const std::string &current_hint : routeParameters.hints)
        {
            json_hints.values.push_back(current_hint);
            ++counter;
        }
        json_result.values["hints"] = json_hints;

        JSON::render(reply.content, json_result);
    }

  private:
    std::string descriptor_string;
};

#endif // HELLO_WORLD_PLUGIN_H
