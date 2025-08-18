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

#include "request_parser.hpp"

#include "http/compression_type.hpp"
#include "http/header.hpp"
#include "http/request.hpp"

#include "../data_structures/tribool.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <string>

namespace http
{

RequestParser::RequestParser()
    : state(internal_state::method_start), current_header({"", ""}),
      selected_compression(no_compression)
{
}

std::tuple<osrm::tribool, compression_type>
RequestParser::parse(request &current_request, char *begin, char *end)
{
    while (begin != end)
    {
        osrm::tribool result = consume(current_request, *begin++);
        if (result != osrm::tribool::indeterminate)
        {
            return std::make_tuple(result, selected_compression);
        }
    }
    osrm::tribool result = osrm::tribool::indeterminate;
    return std::make_tuple(result, selected_compression);
}

osrm::tribool RequestParser::consume(request &current_request, const char input)
{
    switch (state)
    {
    case internal_state::method_start:
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return osrm::tribool::no;
        }
        state = internal_state::method;
        return osrm::tribool::indeterminate;
    case internal_state::method:
        if (input == ' ')
        {
            state = internal_state::uri;
            return osrm::tribool::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return osrm::tribool::no;
        }
        return osrm::tribool::indeterminate;
    case internal_state::uri_start:
        if (is_CTL(input))
        {
            return osrm::tribool::no;
        }
        state = internal_state::uri;
        current_request.uri.push_back(input);
        return osrm::tribool::indeterminate;
    case internal_state::uri:
        if (input == ' ')
        {
            state = internal_state::http_version_h;
            return osrm::tribool::indeterminate;
        }
        if (is_CTL(input))
        {
            return osrm::tribool::no;
        }
        current_request.uri.push_back(input);
        return osrm::tribool::indeterminate;
    case internal_state::http_version_h:
        if (input == 'H')
        {
            state = internal_state::http_version_t_1;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_t_1:
        if (input == 'T')
        {
            state = internal_state::http_version_t_2;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_t_2:
        if (input == 'T')
        {
            state = internal_state::http_version_p;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_p:
        if (input == 'P')
        {
            state = internal_state::http_version_slash;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_slash:
        if (input == '/')
        {
            state = internal_state::http_version_major_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_major_start:
        if (is_digit(input))
        {
            state = internal_state::http_version_major;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_major:
        if (input == '.')
        {
            state = internal_state::http_version_minor_start;
            return osrm::tribool::indeterminate;
        }
        if (is_digit(input))
        {
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_minor_start:
        if (is_digit(input))
        {
            state = internal_state::http_version_minor;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::http_version_minor:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_1;
            return osrm::tribool::indeterminate;
        }
        if (is_digit(input))
        {
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::expecting_newline_1:
        if (input == '\n')
        {
            state = internal_state::header_line_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::header_line_start:
        if (boost::iequals(current_header.name, "Accept-Encoding"))
        {
            /* giving gzip precedence over deflate */
            if (boost::icontains(current_header.value, "deflate"))
            {
                selected_compression = deflate_rfc1951;
            }
            if (boost::icontains(current_header.value, "gzip"))
            {
                selected_compression = gzip_rfc1952;
            }
        }

        if (boost::iequals(current_header.name, "Referer"))
        {
            current_request.referrer = current_header.value;
        }

        if (boost::iequals(current_header.name, "User-Agent"))
        {
            current_request.agent = current_header.value;
        }

        if (input == '\r')
        {
            state = internal_state::expecting_newline_3;
            return osrm::tribool::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return osrm::tribool::no;
        }
        state = internal_state::header_name;
        current_header.clear();
        current_header.name.push_back(input);
        return osrm::tribool::indeterminate;
    case internal_state::header_lws:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_2;
            return osrm::tribool::indeterminate;
        }
        if (input == ' ' || input == '\t')
        {
            return osrm::tribool::indeterminate;
        }
        if (is_CTL(input))
        {
            return osrm::tribool::no;
        }
        state = internal_state::header_value;
        return osrm::tribool::indeterminate;
    case internal_state::header_name:
        if (input == ':')
        {
            state = internal_state::space_before_header_value;
            return osrm::tribool::indeterminate;
        }
        if (!is_char(input) || is_CTL(input) || is_special(input))
        {
            return osrm::tribool::no;
        }
        current_header.name.push_back(input);
        return osrm::tribool::indeterminate;
    case internal_state::space_before_header_value:
        if (input == ' ')
        {
            state = internal_state::header_value;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    case internal_state::header_value:
        if (input == '\r')
        {
            state = internal_state::expecting_newline_2;
            return osrm::tribool::indeterminate;
        }
        if (is_CTL(input))
        {
            return osrm::tribool::no;
        }
        current_header.value.push_back(input);
        return osrm::tribool::indeterminate;
    case internal_state::expecting_newline_2:
        if (input == '\n')
        {
            state = internal_state::header_line_start;
            return osrm::tribool::indeterminate;
        }
        return osrm::tribool::no;
    default: // expecting_newline_3
        return input == '\n' ? osrm::tribool::yes : osrm::tribool::no;
    }
}

bool RequestParser::is_char(const int character) const
{
    return character >= 0 && character <= 127;
}

bool RequestParser::is_CTL(const int character) const
{
    return (character >= 0 && character <= 31) || (character == 127);
}

bool RequestParser::is_special(const int character) const
{
    switch (character)
    {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

bool RequestParser::is_digit(const int character) const
{
    return character >= '0' && character <= '9';
}
}
