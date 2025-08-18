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

#ifndef EXTRACTION_HELPER_FUNCTIONS_HPP
#define EXTRACTION_HELPER_FUNCTIONS_HPP

#include "../util/cast.hpp"
#include "../util/iso_8601_duration_parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/regex.hpp>

#include <limits>

bool simple_duration_is_valid(const std::string &s)
{
    boost::regex simple_format(
        "((\\d|\\d\\d):(\\d|\\d\\d):(\\d|\\d\\d))|((\\d|\\d\\d):(\\d|\\d\\d))|(\\d|\\d\\d)",
        boost::regex_constants::icase | boost::regex_constants::perl);

    const bool simple_matched = regex_match(s, simple_format);

    if (simple_matched)
    {
        return true;
    }
    return false;
}

bool iso_8601_duration_is_valid(const std::string &s)
{
    iso_8601_grammar<std::string::const_iterator> iso_parser;
    const bool result = qi::parse(s.begin(), s.end(), iso_parser);

    // check if the was an error with the request
    if (result && (0 != iso_parser.get_duration()))
    {
        return true;
    }
    return false;
}

bool durationIsValid(const std::string &s)
{
    return simple_duration_is_valid(s) || iso_8601_duration_is_valid(s);
}

unsigned parseDuration(const std::string &s)
{
    if (simple_duration_is_valid(s))
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
                minutes = cast::string_to_int(result[0]);
            }
            if (2 == result.size())
            {
                minutes = cast::string_to_int(result[1]);
                hours = cast::string_to_int(result[0]);
            }
            if (3 == result.size())
            {
                seconds = cast::string_to_int(result[2]);
                minutes = cast::string_to_int(result[1]);
                hours = cast::string_to_int(result[0]);
            }
            return 10 * (3600 * hours + 60 * minutes + seconds);
        }
    }
    else if (iso_8601_duration_is_valid(s))
    {
        iso_8601_grammar<std::string::const_iterator> iso_parser;
        qi::parse(s.begin(), s.end(), iso_parser);

        return iso_parser.get_duration();
    }

    return std::numeric_limits<unsigned>::max();
}

#endif // EXTRACTION_HELPER_FUNCTIONS_HPP
