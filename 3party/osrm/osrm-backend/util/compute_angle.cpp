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

#include "compute_angle.hpp"

#include "trigonometry_table.hpp"
#include "../util/mercator.hpp"

#include <osrm/coordinate.hpp>

#include <cmath>

double ComputeAngle::OfThreeFixedPointCoordinates(const FixedPointCoordinate &first,
                                                  const FixedPointCoordinate &second,
                                                  const FixedPointCoordinate &third) noexcept
{
    const double v1x = (first.lon - second.lon) / COORDINATE_PRECISION;
    const double v1y = mercator::lat2y(first.lat / COORDINATE_PRECISION) -
                       mercator::lat2y(second.lat / COORDINATE_PRECISION);
    const double v2x = (third.lon - second.lon) / COORDINATE_PRECISION;
    const double v2y = mercator::lat2y(third.lat / COORDINATE_PRECISION) -
                       mercator::lat2y(second.lat / COORDINATE_PRECISION);

    double angle = (atan2_lookup(v2y, v2x) - atan2_lookup(v1y, v1x)) * 180. / M_PI;
    while (angle < 0.)
    {
        angle += 360.;
    }
    return angle;
}
