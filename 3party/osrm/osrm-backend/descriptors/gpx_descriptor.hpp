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

#ifndef GPX_DESCRIPTOR_HPP
#define GPX_DESCRIPTOR_HPP

#include "descriptor_base.hpp"
#include "../util/xml_renderer.hpp"

#include <osrm/json_container.hpp>

#include <iostream>

template <class DataFacadeT> class GPXDescriptor final : public BaseDescriptor<DataFacadeT>
{
  private:
    DescriptorConfig config;
    DataFacadeT *facade;

    void AddRoutePoint(const FixedPointCoordinate &coordinate, osrm::json::Array &json_route)
    {
        osrm::json::Object json_lat;
        osrm::json::Object json_lon;
        osrm::json::Array json_row;

        std::string tmp;

        coordinate_calculation::lat_or_lon_to_string(coordinate.lat, tmp);
        json_lat.values["_lat"] = tmp;

        coordinate_calculation::lat_or_lon_to_string(coordinate.lon, tmp);
        json_lon.values["_lon"] = tmp;

        json_row.values.push_back(json_lat);
        json_row.values.push_back(json_lon);
        osrm::json::Object entry;
        entry.values["rtept"] = json_row;
        json_route.values.push_back(entry);
    }

  public:
    explicit GPXDescriptor(DataFacadeT *facade) : facade(facade) {}

    virtual void SetConfig(const DescriptorConfig &c) final { config = c; }

    virtual void Run(const InternalRouteResult &raw_route, osrm::json::Object &json_result) final
    {
        osrm::json::Array json_route;
        if (raw_route.shortest_path_length != INVALID_EDGE_WEIGHT)
        {
            AddRoutePoint(raw_route.segment_end_coordinates.front().source_phantom.location,
                          json_route);

            for (const std::vector<PathData> &path_data_vector : raw_route.unpacked_path_segments)
            {
                for (const PathData &path_data : path_data_vector)
                {
                    const FixedPointCoordinate current_coordinate =
                        facade->GetCoordinateOfNode(path_data.node);
                    AddRoutePoint(current_coordinate, json_route);
                }
            }
            AddRoutePoint(raw_route.segment_end_coordinates.back().target_phantom.location,
                          json_route);
        }
        // osrm::json::gpx_render(reply.content, json_route);
        json_result.values["route"] = json_route;
    }
};
#endif // GPX_DESCRIPTOR_HPP
