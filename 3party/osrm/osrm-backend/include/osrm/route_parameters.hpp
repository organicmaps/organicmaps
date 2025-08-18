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

#ifndef ROUTE_PARAMETERS_HPP
#define ROUTE_PARAMETERS_HPP

#include <osrm/coordinate.hpp>

#include <boost/fusion/container/vector/vector_fwd.hpp>

#include <string>
#include <vector>

struct RouteParameters
{
    RouteParameters();

    void setZoomLevel(const short level);

    void setNumberOfResults(const short number);

    void setAlternateRouteFlag(const bool flag);

    void setUTurn(const bool flag);

    void setAllUTurns(const bool flag);

    void setClassify(const bool classify);

    void setMatchingBeta(const double beta);

    void setGPSPrecision(const double precision);

    void setDeprecatedAPIFlag(const std::string &);

    void setChecksum(const unsigned check_sum);

    void setInstructionFlag(const bool flag);

    void setService(const std::string &service);

    void setOutputFormat(const std::string &format);

    void setJSONpParameter(const std::string &parameter);

    void addHint(const std::string &hint);

    void addTimestamp(const unsigned timestamp);

    void setLanguage(const std::string &language);

    void setGeometryFlag(const bool flag);

    void setCompressionFlag(const bool flag);

    void addCoordinate(const boost::fusion::vector<double, double> &received_coordinates);

    short zoom_level;
    bool print_instructions;
    bool alternate_route;
    bool geometry;
    bool compression;
    bool deprecatedAPI;
    bool uturn_default;
    bool classify;
    double matching_beta;
    double gps_precision;
    unsigned check_sum;
    short num_results;
    std::string service;
    std::string output_format;
    std::string jsonp_parameter;
    std::string language;
    std::vector<std::string> hints;
    std::vector<unsigned> timestamps;
    std::vector<bool> uturns;
    std::vector<FixedPointCoordinate> coordinates;
};

#endif // ROUTE_PARAMETERS_HPP
