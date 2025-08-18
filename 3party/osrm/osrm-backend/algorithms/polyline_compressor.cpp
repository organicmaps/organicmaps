/*

Copyright (c) 2014, Project OSRM contributors
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

#include "polyline_compressor.hpp"
#include "../data_structures/segment_information.hpp"

#include <osrm/coordinate.hpp>

std::string PolylineCompressor::encode_vector(std::vector<int> &numbers) const
{
    std::string output;
    const auto end = numbers.size();
    for (std::size_t i = 0; i < end; ++i)
    {
        numbers[i] <<= 1;
        if (numbers[i] < 0)
        {
            numbers[i] = ~(numbers[i]);
        }
    }
    for (const int number : numbers)
    {
        output += encode_number(number);
    }
    return output;
}

std::string PolylineCompressor::encode_number(int number_to_encode) const
{
    std::string output;
    while (number_to_encode >= 0x20)
    {
        const int next_value = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(next_value);
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
    return output;
}

std::string
PolylineCompressor::get_encoded_string(const std::vector<SegmentInformation> &polyline) const
{
    if (polyline.empty())
    {
        return {};
    }

    std::vector<int> delta_numbers;
    delta_numbers.reserve((polyline.size() - 1) * 2);
    FixedPointCoordinate previous_coordinate = {0, 0};
    for (const auto &segment : polyline)
    {
        if (segment.necessary)
        {
            const int lat_diff = segment.location.lat - previous_coordinate.lat;
            const int lon_diff = segment.location.lon - previous_coordinate.lon;
            delta_numbers.emplace_back(lat_diff);
            delta_numbers.emplace_back(lon_diff);
            previous_coordinate = segment.location;
        }
    }
    return encode_vector(delta_numbers);
}
