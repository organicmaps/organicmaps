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

#ifndef EXTRACTION_HELPER_FUNCTIONS_H
#define EXTRACTION_HELPER_FUNCTIONS_H

#include "../Util/StringUtil.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/regex.hpp>

#include <limits>

namespace qi = boost::spirit::qi;

// TODO: Move into LUA

inline bool durationIsValid(const std::string &s)
{
    boost::regex e(
        "((\\d|\\d\\d):(\\d|\\d\\d):(\\d|\\d\\d))|((\\d|\\d\\d):(\\d|\\d\\d))|(\\d|\\d\\d)",
        boost::regex_constants::icase | boost::regex_constants::perl);

    std::vector<std::string> result;
    boost::algorithm::split_regex(result, s, boost::regex(":"));
    const bool matched = regex_match(s, e);
    return matched;
}

inline unsigned parseDuration(const std::string &s)
{
    unsigned hours = 0;
    unsigned minutes = 0;
    unsigned seconds = 0;
    boost::regex e(
        "((\\d|\\d\\d):(\\d|\\d\\d):(\\d|\\d\\d))|((\\d|\\d\\d):(\\d|\\d\\d))|(\\d|\\d\\d)",
        boost::regex_constants::icase | boost::regex_constants::perl);

    std::vector<std::string> result;
    boost::algorithm::split_regex(result, s, boost::regex(":"));
    const bool matched = regex_match(s, e);
    if (matched)
    {
        if (1 == result.size())
        {
            minutes = StringToUint(result[0]);
        }
        if (2 == result.size())
        {
            minutes = StringToUint(result[1]);
            hours = StringToUint(result[0]);
        }
        if (3 == result.size())
        {
            seconds = StringToUint(result[2]);
            minutes = StringToUint(result[1]);
            hours = StringToUint(result[0]);
        }
        return 10 * (3600 * hours + 60 * minutes + seconds);
    }
    return std::numeric_limits<unsigned>::max();
}

#endif // EXTRACTION_HELPER_FUNCTIONS_H_
