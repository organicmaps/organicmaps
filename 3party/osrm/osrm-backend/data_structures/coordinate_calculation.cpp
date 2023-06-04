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

#include "../util/mercator.hpp"
#include "../util/string_util.hpp"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>

#include <cmath>

#include <limits>

namespace
{
constexpr static const float RAD = 0.017453292519943295769236907684886f;
// earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
// The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
constexpr static const float earth_radius = 6372797.560856f;
}

double coordinate_calculation::great_circle_distance(const int lat1,
                                                     const int lon1,
                                                     const int lat2,
                                                     const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());
    const double lt1 = lat1 / COORDINATE_PRECISION;
    const double ln1 = lon1 / COORDINATE_PRECISION;
    const double lt2 = lat2 / COORDINATE_PRECISION;
    const double ln2 = lon2 / COORDINATE_PRECISION;
    const double dlat1 = lt1 * (RAD);

    const double dlong1 = ln1 * (RAD);
    const double dlat2 = lt2 * (RAD);
    const double dlong2 = ln2 * (RAD);

    const double dLong = dlong1 - dlong2;
    const double dLat = dlat1 - dlat2;

    const double aHarv = std::pow(std::sin(dLat / 2.0), 2.0) +
                         std::cos(dlat1) * std::cos(dlat2) * std::pow(std::sin(dLong / 2.), 2);
    const double cHarv = 2. * std::atan2(std::sqrt(aHarv), std::sqrt(1.0 - aHarv));
    return earth_radius * cHarv;
}

double coordinate_calculation::great_circle_distance(const FixedPointCoordinate &coordinate_1,
                                                     const FixedPointCoordinate &coordinate_2)
{
    return great_circle_distance(coordinate_1.lat, coordinate_1.lon, coordinate_2.lat,
                                 coordinate_2.lon);
}

float coordinate_calculation::euclidean_distance(const FixedPointCoordinate &coordinate_1,
                                                 const FixedPointCoordinate &coordinate_2)
{
    return euclidean_distance(coordinate_1.lat, coordinate_1.lon, coordinate_2.lat,
                              coordinate_2.lon);
}

float coordinate_calculation::euclidean_distance(const int lat1,
                                                 const int lon1,
                                                 const int lat2,
                                                 const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());

    const float float_lat1 = (lat1 / COORDINATE_PRECISION) * RAD;
    const float float_lon1 = (lon1 / COORDINATE_PRECISION) * RAD;
    const float float_lat2 = (lat2 / COORDINATE_PRECISION) * RAD;
    const float float_lon2 = (lon2 / COORDINATE_PRECISION) * RAD;

    const float x_value = (float_lon2 - float_lon1) * std::cos((float_lat1 + float_lat2) / 2.f);
    const float y_value = float_lat2 - float_lat1;
    return std::hypot(x_value, y_value) * earth_radius;
}

float coordinate_calculation::perpendicular_distance(const FixedPointCoordinate &source_coordinate,
                                                     const FixedPointCoordinate &target_coordinate,
                                                     const FixedPointCoordinate &query_location)
{
    float ratio;
    FixedPointCoordinate nearest_location;

    return perpendicular_distance(source_coordinate, target_coordinate, query_location,
                                  nearest_location, ratio);
}

float coordinate_calculation::perpendicular_distance(const FixedPointCoordinate &segment_source,
                                                     const FixedPointCoordinate &segment_target,
                                                     const FixedPointCoordinate &query_location,
                                                     FixedPointCoordinate &nearest_location,
                                                     float &ratio)
{
    return perpendicular_distance_from_projected_coordinate(
        segment_source, segment_target, query_location,
        {mercator::lat2y(query_location.lat / COORDINATE_PRECISION),
         query_location.lon / COORDINATE_PRECISION},
        nearest_location, ratio);
}

float coordinate_calculation::perpendicular_distance_from_projected_coordinate(
    const FixedPointCoordinate &source_coordinate,
    const FixedPointCoordinate &target_coordinate,
    const FixedPointCoordinate &query_location,
    const std::pair<double, double> &projected_coordinate)
{
    float ratio;
    FixedPointCoordinate nearest_location;

    return perpendicular_distance_from_projected_coordinate(source_coordinate, target_coordinate,
                                                            query_location, projected_coordinate,
                                                            nearest_location, ratio);
}

float coordinate_calculation::perpendicular_distance_from_projected_coordinate(
    const FixedPointCoordinate &segment_source,
    const FixedPointCoordinate &segment_target,
    const FixedPointCoordinate &query_location,
    const std::pair<double, double> &projected_coordinate,
    FixedPointCoordinate &nearest_location,
    float &ratio)
{
    BOOST_ASSERT(query_location.is_valid());

    // initialize values
    const double x = projected_coordinate.first;
    const double y = projected_coordinate.second;
    const double a = mercator::lat2y(segment_source.lat / COORDINATE_PRECISION);
    const double b = segment_source.lon / COORDINATE_PRECISION;
    const double c = mercator::lat2y(segment_target.lat / COORDINATE_PRECISION);
    const double d = segment_target.lon / COORDINATE_PRECISION;
    double p, q /*,mX*/, nY;
    if (std::abs(a - c) > std::numeric_limits<double>::epsilon())
    {
        const double m = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m * y)) + (m * m * a - m * b)) / (1.f + m * m);
        q = b + m * (p - a);
    }
    else
    {
        p = c;
        q = y;
    }
    nY = (d * p - c * q) / (a * d - b * c);

    // discretize the result to coordinate precision. it's a hack!
    if (std::abs(nY) < (1.f / COORDINATE_PRECISION))
    {
        nY = 0.f;
    }

    // compute ratio
    ratio =
        static_cast<float>((p - nY * a) / c); // These values are actually n/m+n and m/m+n , we need
    // not calculate the explicit values of m an n as we
    // are just interested in the ratio
    if (std::isnan(ratio))
    {
        ratio = (segment_target == query_location ? 1.f : 0.f);
    }
    else if (std::abs(ratio) <= std::numeric_limits<float>::epsilon())
    {
        ratio = 0.f;
    }
    else if (std::abs(ratio - 1.f) <= std::numeric_limits<float>::epsilon())
    {
        ratio = 1.f;
    }

    // compute nearest location
    BOOST_ASSERT(!std::isnan(ratio));
    if (ratio <= 0.f)
    {
        nearest_location = segment_source;
    }
    else if (ratio >= 1.f)
    {
        nearest_location = segment_target;
    }
    else
    {
        // point lies in between
        nearest_location.lat = static_cast<int>(mercator::y2lat(p) * COORDINATE_PRECISION);
        nearest_location.lon = static_cast<int>(q * COORDINATE_PRECISION);
    }
    BOOST_ASSERT(nearest_location.is_valid());

    const float approximate_distance =
        coordinate_calculation::euclidean_distance(query_location, nearest_location);
    BOOST_ASSERT(0.f <= approximate_distance);
    return approximate_distance;
}

void coordinate_calculation::lat_or_lon_to_string(const int value, std::string &output)
{
    char buffer[12];
    buffer[11] = 0; // zero termination
    output = printInt<11, 6>(buffer, value);
}

float coordinate_calculation::deg_to_rad(const float degree)
{
    return degree * (static_cast<float>(M_PI) / 180.f);
}

float coordinate_calculation::rad_to_deg(const float radian)
{
    return radian * (180.f * static_cast<float>(M_1_PI));
}

float coordinate_calculation::bearing(const FixedPointCoordinate &first_coordinate,
                                      const FixedPointCoordinate &second_coordinate)
{
    const float lon_diff =
        second_coordinate.lon / COORDINATE_PRECISION - first_coordinate.lon / COORDINATE_PRECISION;
    const float lon_delta = deg_to_rad(lon_diff);
    const float lat1 = deg_to_rad(first_coordinate.lat / COORDINATE_PRECISION);
    const float lat2 = deg_to_rad(second_coordinate.lat / COORDINATE_PRECISION);
    const float y = std::sin(lon_delta) * std::cos(lat2);
    const float x =
        std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(lon_delta);
    float result = rad_to_deg(std::atan2(y, x));
    while (result < 0.f)
    {
        result += 360.f;
    }

    while (result >= 360.f)
    {
        result -= 360.f;
    }
    return result;
}
