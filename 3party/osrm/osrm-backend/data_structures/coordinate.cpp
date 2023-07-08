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

#include "coordinate_calculation.hpp"

#include <osrm/coordinate.hpp>

#include <iostream>
#include <limits>

FixedPointCoordinate::FixedPointCoordinate()
    : lat(std::numeric_limits<int>::min()), lon(std::numeric_limits<int>::min())
{
}

FixedPointCoordinate::FixedPointCoordinate(int lat, int lon) : lat(lat), lon(lon)
{
}

bool FixedPointCoordinate::is_valid() const
{
    if (lat > 90 * COORDINATE_PRECISION || lat < -90 * COORDINATE_PRECISION ||
        lon > 180 * COORDINATE_PRECISION || lon < -180 * COORDINATE_PRECISION)
    {
        return false;
    }
    return true;
}

bool FixedPointCoordinate::operator==(const FixedPointCoordinate &other) const
{
    return lat == other.lat && lon == other.lon;
}

void FixedPointCoordinate::output(std::ostream &out) const
{
    out << "(" << lat / COORDINATE_PRECISION << "," << lon / COORDINATE_PRECISION << ")";
}

float FixedPointCoordinate::bearing(const FixedPointCoordinate &other) const
{
    return coordinate_calculation::bearing(other, *this);
}
