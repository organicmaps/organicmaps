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

#include <osrm/Coordinate.h>

#include "../Util/MercatorUtil.h"
#include "../Util/StringUtil.h"

#include <boost/assert.hpp>

#include <iostream>
#include <limits>


FixedPointCoordinate::FixedPointCoordinate()
    : lat(std::numeric_limits<int>::min()), lon(std::numeric_limits<int>::min())
{
}

FixedPointCoordinate::FixedPointCoordinate(int lat, int lon) : lat(lat), lon(lon)
{
}

void FixedPointCoordinate::Reset()
{
    lat = std::numeric_limits<int>::min();
    lon = std::numeric_limits<int>::min();
}
bool FixedPointCoordinate::isSet() const
{
    return (std::numeric_limits<int>::min() != lat) && (std::numeric_limits<int>::min() != lon);
}
bool FixedPointCoordinate::isValid() const
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

double FixedPointCoordinate::ApproximateDistance(const int lat1,
                                                 const int lon1,
                                                 const int lat2,
                                                 const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());
    double RAD = 0.017453292519943295769236907684886;
    double lt1 = lat1 / COORDINATE_PRECISION;
    double ln1 = lon1 / COORDINATE_PRECISION;
    double lt2 = lat2 / COORDINATE_PRECISION;
    double ln2 = lon2 / COORDINATE_PRECISION;
    double dlat1 = lt1 * (RAD);

    double dlong1 = ln1 * (RAD);
    double dlat2 = lt2 * (RAD);
    double dlong2 = ln2 * (RAD);

    double dLong = dlong1 - dlong2;
    double dLat = dlat1 - dlat2;

    double aHarv = pow(sin(dLat / 2.0), 2.0) + cos(dlat1) * cos(dlat2) * pow(sin(dLong / 2.), 2);
    double cHarv = 2. * atan2(sqrt(aHarv), sqrt(1.0 - aHarv));
    // earth radius varies between 6,356.750-6,378.135 km (3,949.901-3,963.189mi)
    // The IUGG value for the equatorial radius is 6378.137 km (3963.19 miles)
    const double earth = 6372797.560856;
    return earth * cHarv;
}

double FixedPointCoordinate::ApproximateDistance(const FixedPointCoordinate &coordinate_1,
                                                 const FixedPointCoordinate &coordinate_2)
{
    return ApproximateDistance(
        coordinate_1.lat, coordinate_1.lon, coordinate_2.lat, coordinate_2.lon);
}

float FixedPointCoordinate::ApproximateEuclideanDistance(const FixedPointCoordinate &coordinate_1,
                                                         const FixedPointCoordinate &coordinate_2)
{
    return ApproximateEuclideanDistance(
        coordinate_1.lat, coordinate_1.lon, coordinate_2.lat, coordinate_2.lon);
}

float FixedPointCoordinate::ApproximateEuclideanDistance(const int lat1,
                                                         const int lon1,
                                                         const int lat2,
                                                         const int lon2)
{
    BOOST_ASSERT(lat1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon1 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lat2 != std::numeric_limits<int>::min());
    BOOST_ASSERT(lon2 != std::numeric_limits<int>::min());

    const float RAD = 0.017453292519943295769236907684886f;
    const float float_lat1 = (lat1 / COORDINATE_PRECISION) * RAD;
    const float float_lon1 = (lon1 / COORDINATE_PRECISION) * RAD;
    const float float_lat2 = (lat2 / COORDINATE_PRECISION) * RAD;
    const float float_lon2 = (lon2 / COORDINATE_PRECISION) * RAD;

    const float x_value = (float_lon2 - float_lon1) * cos((float_lat1 + float_lat2) / 2.f);
    const float y_value = float_lat2 - float_lat1;
    const float earth_radius = 6372797.560856f;
    return sqrt(x_value * x_value + y_value * y_value) * earth_radius;
}

float
FixedPointCoordinate::ComputePerpendicularDistance(const FixedPointCoordinate &source_coordinate,
                                                   const FixedPointCoordinate &target_coordinate,
                                                   const FixedPointCoordinate &point)
{
    // initialize values
    const float x_value = static_cast<float>(lat2y(point.lat / COORDINATE_PRECISION));
    const float y_value = point.lon / COORDINATE_PRECISION;
    float a = static_cast<float>(lat2y(source_coordinate.lat / COORDINATE_PRECISION));
    float b = source_coordinate.lon / COORDINATE_PRECISION;
    float c = static_cast<float>(lat2y(target_coordinate.lat / COORDINATE_PRECISION));
    float d = target_coordinate.lon / COORDINATE_PRECISION;
    float p, q;
    if (std::abs(a - c) > std::numeric_limits<float>::epsilon())
    {
        const float slope = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x_value + (slope * y_value)) + (slope * slope * a - slope * b)) /
            (1.f + slope * slope);
        q = b + slope * (p - a);
    }
    else
    {
        p = c;
        q = y_value;
    }

    float ratio;
    bool inverse_ratio = false;

    // straight line segment on equator
    if (std::abs(c) < std::numeric_limits<float>::epsilon() &&
        std::abs(a) < std::numeric_limits<float>::epsilon())
    {
        ratio = (q - b) / (d - b);
    }
    else
    {
        if (std::abs(c) < std::numeric_limits<float>::epsilon())
        {
            // swap start/end
            std::swap(a, c);
            std::swap(b, d);
            inverse_ratio = true;
        }

        float nY = (d * p - c * q) / (a * d - b * c);
        // discretize the result to coordinate precision. it's a hack!
        if (std::abs(nY) < (1.f / COORDINATE_PRECISION))
        {
            nY = 0.f;
        }

        // compute ratio
        ratio = (p - nY * a) / c;
    }

    if (std::isnan(ratio))
    {
        ratio = (target_coordinate == point ? 1.f : 0.f);
    }
    else if (std::abs(ratio) <= std::numeric_limits<float>::epsilon())
    {
        ratio = 0.f;
    }
    else if (std::abs(ratio - 1.f) <= std::numeric_limits<float>::epsilon())
    {
        ratio = 1.f;
    }

    // we need to do this, if we switched start/end coordinates
    if (inverse_ratio)
    {
        ratio = 1.0f - ratio;
    }

    // compute the nearest location
    FixedPointCoordinate nearest_location;
    BOOST_ASSERT(!std::isnan(ratio));
    if (ratio <= 0.f)
    { // point is "left" of edge
        nearest_location = source_coordinate;
    }
    else if (ratio >= 1.f)
    { // point is "right" of edge
        nearest_location = target_coordinate;
    }
    else
    { // point lies in between
        nearest_location.lat = static_cast<int>(y2lat(p) * COORDINATE_PRECISION);
        nearest_location.lon = static_cast<int>(q * COORDINATE_PRECISION);
    }

    BOOST_ASSERT(nearest_location.isValid());
    return FixedPointCoordinate::ApproximateEuclideanDistance(point, nearest_location);
}

float FixedPointCoordinate::ComputePerpendicularDistance(const FixedPointCoordinate &segment_source,
                                                         const FixedPointCoordinate &segment_target,
                                                         const FixedPointCoordinate &query_location,
                                                         FixedPointCoordinate &nearest_location,
                                                         float &ratio)
{
    BOOST_ASSERT(query_location.isValid());

    // initialize values
    const double x = lat2y(query_location.lat / COORDINATE_PRECISION);
    const double y = query_location.lon / COORDINATE_PRECISION;
    const double a = lat2y(segment_source.lat / COORDINATE_PRECISION);
    const double b = segment_source.lon / COORDINATE_PRECISION;
    const double c = lat2y(segment_target.lat / COORDINATE_PRECISION);
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
    ratio = (p - nY * a) / c; // These values are actually n/m+n and m/m+n , we need
    // not calculate the explicit values of m an n as we
    // are just interested in the ratio
    if (std::isnan(ratio))
    {
        ratio = (segment_target == query_location ? 1.f : 0.f);
    }
    else if (std::abs(ratio) <= std::numeric_limits<double>::epsilon())
    {
        ratio = 0.f;
    }
    else if (std::abs(ratio - 1.f) <= std::numeric_limits<double>::epsilon())
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
        nearest_location.lat = static_cast<int>(y2lat(p) * COORDINATE_PRECISION);
        nearest_location.lon = static_cast<int>(q * COORDINATE_PRECISION);
    }
    BOOST_ASSERT(nearest_location.isValid());

    const float approximate_distance =
        FixedPointCoordinate::ApproximateEuclideanDistance(query_location, nearest_location);
    BOOST_ASSERT(0. <= approximate_distance);
    return approximate_distance;
}

void FixedPointCoordinate::convertInternalLatLonToString(const int value, std::string &output)
{
    char buffer[12];
    buffer[11] = 0; // zero termination
    output = printInt<11, 6>(buffer, value);
}

void FixedPointCoordinate::convertInternalCoordinateToString(const FixedPointCoordinate &coord,
                                                             std::string &output)
{
    std::string tmp;
    tmp.reserve(23);
    convertInternalLatLonToString(coord.lon, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lat, tmp);
    output += tmp;
}

void
FixedPointCoordinate::convertInternalReversedCoordinateToString(const FixedPointCoordinate &coord,
                                                                std::string &output)
{
    std::string tmp;
    tmp.reserve(23);
    convertInternalLatLonToString(coord.lat, tmp);
    output = tmp;
    output += ",";
    convertInternalLatLonToString(coord.lon, tmp);
    output += tmp;
}

void FixedPointCoordinate::Output(std::ostream &out) const
{
    out << "(" << lat / COORDINATE_PRECISION << "," << lon / COORDINATE_PRECISION << ")";
}

float FixedPointCoordinate::GetBearing(const FixedPointCoordinate &first_coordinate,
                                       const FixedPointCoordinate &second_coordinate)
{
    const float lon_diff =
        second_coordinate.lon / COORDINATE_PRECISION - first_coordinate.lon / COORDINATE_PRECISION;
    const float lon_delta = DegreeToRadian(lon_diff);
    const float lat1 = DegreeToRadian(first_coordinate.lat / COORDINATE_PRECISION);
    const float lat2 = DegreeToRadian(second_coordinate.lat / COORDINATE_PRECISION);
    const float y = sin(lon_delta) * cos(lat2);
    const float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon_delta);
    float result = RadianToDegree(std::atan2(y, x));
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

float FixedPointCoordinate::GetBearing(const FixedPointCoordinate &other) const
{
    const float lon_delta =
        DegreeToRadian(lon / COORDINATE_PRECISION - other.lon / COORDINATE_PRECISION);
    const float lat1 = DegreeToRadian(other.lat / COORDINATE_PRECISION);
    const float lat2 = DegreeToRadian(lat / COORDINATE_PRECISION);
    const float y_value = std::sin(lon_delta) * std::cos(lat2);
    const float x_value =
        std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(lon_delta);
    float result = RadianToDegree(std::atan2(y_value, x_value));

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

float FixedPointCoordinate::DegreeToRadian(const float degree)
{
    return degree * (static_cast<float>(M_PI) / 180.f);
}

float FixedPointCoordinate::RadianToDegree(const float radian)
{
    return radian * (180.f * static_cast<float>(M_1_PI));
}

// This distance computation does integer arithmetic only and is a lot faster than
// the other distance function which are numerically correct('ish).
// It preserves some order among the elements that make it useful for certain purposes
int FixedPointCoordinate::OrderedPerpendicularDistanceApproximation(
    const FixedPointCoordinate &input_point,
    const FixedPointCoordinate &segment_source,
    const FixedPointCoordinate &segment_target)
{
    // initialize values
    const float x = static_cast<float>(lat2y(input_point.lat / COORDINATE_PRECISION));
    const float y = input_point.lon / COORDINATE_PRECISION;
    const float a = static_cast<float>(lat2y(segment_source.lat / COORDINATE_PRECISION));
    const float b = segment_source.lon / COORDINATE_PRECISION;
    const float c = static_cast<float>(lat2y(segment_target.lat / COORDINATE_PRECISION));
    const float d = segment_target.lon / COORDINATE_PRECISION;

    float p, q;
    if (a == c)
    {
        p = c;
        q = y;
    }
    else
    {
        const float m = (d - b) / (c - a); // slope
        // Projection of (x,y) on line joining (a,b) and (c,d)
        p = ((x + (m * y)) + (m * m * a - m * b)) / (1.f + m * m);
        q = b + m * (p - a);
    }

    const float nY = (d * p - c * q) / (a * d - b * c);
    float ratio = (p - nY * a) / c; // These values are actually n/m+n and m/m+n , we need
    // not calculate the explicit values of m an n as we
    // are just interested in the ratio
    if (std::isnan(ratio))
    {
        ratio = (segment_target == input_point) ? 1.f : 0.f;
    }

    // compute target quasi-location
    int dx, dy;
    if (ratio < 0.f)
    {
        dx = input_point.lon - segment_source.lon;
        dy = input_point.lat - segment_source.lat;
    }
    else if (ratio > 1.f)
    {
        dx = input_point.lon - segment_target.lon;
        dy = input_point.lat - segment_target.lat;
    }
    else
    {
        // point lies in between
        dx = input_point.lon - static_cast<int>(q * COORDINATE_PRECISION);
        dy = input_point.lat - static_cast<int>(y2lat(p) * COORDINATE_PRECISION);
    }

    // return an approximation in the plane
    return static_cast<int>(sqrt(dx * dx + dy * dy));
}
