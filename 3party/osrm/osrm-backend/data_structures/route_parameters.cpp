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

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <osrm/route_parameters.hpp>

RouteParameters::RouteParameters()
    : zoom_level(18), print_instructions(false), alternate_route(true), geometry(true),
      compression(true), deprecatedAPI(false), uturn_default(false), classify(false),
      matching_beta(-1.0), gps_precision(-1.0), check_sum(-1), num_results(1)
{
}

void RouteParameters::setZoomLevel(const short level)
{
    if (18 >= level && 0 <= level)
    {
        zoom_level = level;
    }
}

void RouteParameters::setNumberOfResults(const short number)
{
    if (number > 0 && number <= 100)
    {
        num_results = number;
    }
}

void RouteParameters::setAlternateRouteFlag(const bool flag) { alternate_route = flag; }

void RouteParameters::setUTurn(const bool flag)
{
    uturns.resize(coordinates.size(), uturn_default);
    if (!uturns.empty())
    {
        uturns.back() = flag;
    }
}

void RouteParameters::setAllUTurns(const bool flag)
{
    // if the flag flips the default, then we erase everything.
    if (flag)
    {
        uturn_default = flag;
        uturns.clear();
        uturns.resize(coordinates.size(), uturn_default);
    }
}

void RouteParameters::setDeprecatedAPIFlag(const std::string &) { deprecatedAPI = true; }

void RouteParameters::setChecksum(const unsigned sum) { check_sum = sum; }

void RouteParameters::setInstructionFlag(const bool flag) { print_instructions = flag; }

void RouteParameters::setService(const std::string &service_string) { service = service_string; }

void RouteParameters::setClassify(const bool flag) { classify = flag; }

void RouteParameters::setMatchingBeta(const double beta) { matching_beta = beta; }

void RouteParameters::setGPSPrecision(const double precision) { gps_precision = precision; }

void RouteParameters::setOutputFormat(const std::string &format) { output_format = format; }

void RouteParameters::setJSONpParameter(const std::string &parameter)
{
    jsonp_parameter = parameter;
}

void RouteParameters::addHint(const std::string &hint)
{
    hints.resize(coordinates.size());
    if (!hints.empty())
    {
        hints.back() = hint;
    }
}

void RouteParameters::addTimestamp(const unsigned timestamp)
{
    timestamps.resize(coordinates.size());
    if (!timestamps.empty())
    {
        timestamps.back() = timestamp;
    }
}

void RouteParameters::setLanguage(const std::string &language_string)
{
    language = language_string;
}

void RouteParameters::setGeometryFlag(const bool flag) { geometry = flag; }

void RouteParameters::setCompressionFlag(const bool flag) { compression = flag; }

void RouteParameters::addCoordinate(
    const boost::fusion::vector<double, double> &received_coordinates)
{
    coordinates.emplace_back(
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<0>(received_coordinates)),
        static_cast<int>(COORDINATE_PRECISION * boost::fusion::at_c<1>(received_coordinates)));
}
