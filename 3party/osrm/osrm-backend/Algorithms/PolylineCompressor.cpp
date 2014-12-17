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

#include "PolylineCompressor.h"
#include "../DataStructures/SegmentInformation.h"

#include <osrm/Coordinate.h>

void PolylineCompressor::encodeVectorSignedNumber(std::vector<int> &numbers,
                                                  std::string &output) const
{
    const unsigned end = static_cast<unsigned>(numbers.size());
    for (unsigned i = 0; i < end; ++i)
    {
        numbers[i] <<= 1;
        if (numbers[i] < 0)
        {
            numbers[i] = ~(numbers[i]);
        }
    }
    for (const int number : numbers)
    {
        encodeNumber(number, output);
    }
}

void PolylineCompressor::encodeNumber(int number_to_encode, std::string &output) const
{
    while (number_to_encode >= 0x20)
    {
        const int next_value = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(next_value);
        if (92 == next_value)
        {
            output += static_cast<char>(next_value);
        }
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
    if (92 == number_to_encode)
    {
        output += static_cast<char>(number_to_encode);
    }
}

JSON::String
PolylineCompressor::printEncodedString(const std::vector<SegmentInformation> &polyline) const
{
    std::string output;
    std::vector<int> delta_numbers;
    if (!polyline.empty())
    {
        FixedPointCoordinate last_coordinate = {0, 0};
        for (const auto &segment : polyline)
        {
            if (segment.necessary)
            {
                const int lat_diff = segment.location.lat - last_coordinate.lat;
                const int lon_diff = segment.location.lon - last_coordinate.lon;
                delta_numbers.emplace_back(lat_diff);
                delta_numbers.emplace_back(lon_diff);
                last_coordinate = segment.location;
            }
        }
        encodeVectorSignedNumber(delta_numbers, output);
    }
    JSON::String return_value(output);
    return return_value;
}

JSON::Array
PolylineCompressor::printUnencodedString(const std::vector<SegmentInformation> &polyline) const
{
    JSON::Array json_geometry_array;
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            JSON::Array json_coordinate;
            json_coordinate.values.push_back(segment.location.lat / COORDINATE_PRECISION);
            json_coordinate.values.push_back(segment.location.lon / COORDINATE_PRECISION);
            json_geometry_array.values.push_back(json_coordinate);
        }
    }
    return json_geometry_array;
}
