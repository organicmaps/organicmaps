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

#ifndef GPX_DESCRIPTOR_H
#define GPX_DESCRIPTOR_H

#include "BaseDescriptor.h"

template <class DataFacadeT> class GPXDescriptor final : public BaseDescriptor<DataFacadeT>
{
  private:
    DescriptorConfig config;
    FixedPointCoordinate current;
    DataFacadeT *facade;

    void AddRoutePoint(const FixedPointCoordinate &coordinate, std::vector<char> &output)
    {
        const std::string route_point_head = "<rtept lat=\"";
        const std::string route_point_middle = " lon=\"";
        const std::string route_point_tail = "\"></rtept>";

        std::string tmp;

        FixedPointCoordinate::convertInternalLatLonToString(coordinate.lat, tmp);
        output.insert(output.end(), route_point_head.begin(), route_point_head.end());
        output.insert(output.end(), tmp.begin(), tmp.end());
        output.push_back('\"');

        FixedPointCoordinate::convertInternalLatLonToString(coordinate.lon, tmp);
        output.insert(output.end(), route_point_middle.begin(), route_point_middle.end());
        output.insert(output.end(), tmp.begin(), tmp.end());
        output.insert(output.end(), route_point_tail.begin(), route_point_tail.end());
    }

  public:
    explicit GPXDescriptor(DataFacadeT *facade) : facade(facade) {}

    void SetConfig(const DescriptorConfig &c) final { config = c; }

    // TODO: reorder parameters
    void Run(const RawRouteData &raw_route, http::Reply &reply) final
    {
        std::string header("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                           "<gpx creator=\"OSRM Routing Engine\" version=\"1.1\" "
                           "xmlns=\"http://www.topografix.com/GPX/1/1\" "
                           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                           "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 gpx.xsd"
                           "\">"
                           "<metadata><copyright author=\"Project OSRM\"><license>Data (c)"
                           " OpenStreetMap contributors (ODbL)</license></copyright>"
                           "</metadata>"
                           "<rte>");
        reply.content.insert(reply.content.end(), header.begin(), header.end());
        const bool found_route = (raw_route.shortest_path_length != INVALID_EDGE_WEIGHT) &&
                                 (!raw_route.unpacked_path_segments.front().empty());
        if (found_route)
        {
            AddRoutePoint(raw_route.segment_end_coordinates.front().source_phantom.location,
                          reply.content);

            for (const std::vector<PathData> &path_data_vector : raw_route.unpacked_path_segments)
            {
                for (const PathData &path_data : path_data_vector)
                {
                    const FixedPointCoordinate current_coordinate =
                        facade->GetCoordinateOfNode(path_data.node);
                    AddRoutePoint(current_coordinate, reply.content);
                }
            }
            AddRoutePoint(raw_route.segment_end_coordinates.back().target_phantom.location,
                          reply.content);
        }
        std::string footer("</rte></gpx>");
        reply.content.insert(reply.content.end(), footer.begin(), footer.end());
    }
};
#endif // GPX_DESCRIPTOR_H
