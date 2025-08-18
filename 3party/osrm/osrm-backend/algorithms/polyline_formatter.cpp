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

#include "polyline_formatter.hpp"

#include "polyline_compressor.hpp"
#include "../data_structures/segment_information.hpp"

#include <osrm/coordinate.hpp>

osrm::json::String
PolylineFormatter::printEncodedString(const std::vector<SegmentInformation> &polyline) const
{
    return osrm::json::String(PolylineCompressor().get_encoded_string(polyline));
}

osrm::json::Array
PolylineFormatter::printUnencodedString(const std::vector<SegmentInformation> &polyline) const
{
    osrm::json::Array json_geometry_array;
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            osrm::json::Array json_coordinate;
            json_coordinate.values.push_back(segment.location.lat / COORDINATE_PRECISION);
            json_coordinate.values.push_back(segment.location.lon / COORDINATE_PRECISION);
            json_geometry_array.values.push_back(json_coordinate);
        }
    }
    return json_geometry_array;
}
