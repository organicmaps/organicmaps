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

#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <boost/algorithm/string.hpp>

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>

// precision:  position after decimal point
// length: maximum number of digits including comma and decimals
// work with negative values to prevent overflowing when taking -value
template <int length, int precision> static inline char *printInt(char *buffer, int value)
{
    bool minus = true;
    if (value > 0)
    {
        minus = false;
        value = -value;
    }
    buffer += length - 1;
    for (int i = 0; i < precision; i++)
    {
        *buffer = '0' - (value % 10);
        value /= 10;
        buffer--;
    }
    *buffer = '.';
    buffer--;
    for (int i = precision + 1; i < length; i++)
    {
        *buffer = '0' - (value % 10);
        value /= 10;
        if (value == 0)
            break;
        buffer--;
    }
    if (minus)
    {
        buffer--;
        *buffer = '-';
    }
    return buffer;
}

inline void replaceAll(std::string &s, const std::string &sub, const std::string &other)
{
    boost::replace_all(s, sub, other);
}

inline std::string EscapeJSONString(const std::string &input)
{
    std::string output;
    output.reserve(input.size());
    for (auto iter = input.begin(); iter != input.end(); ++iter)
    {
        switch (iter[0])
        {
        case '\\':
            output += "\\\\";
            break;
        case '"':
            output += "\\\"";
            break;
        case '/':
            output += "\\/";
            break;
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output += *iter;
            break;
        }
    }
    return output;
}

static std::string originals[] = {"&", "\"", "<", ">", "'", "[", "]", "\\"};
static std::string entities[] = {
    "&amp;", "&quot;", "&lt;", "&gt;", "&#39;", "&91;", "&93;", " &#92;"};

inline std::size_t URIDecode(const std::string &input, std::string &output)
{
    auto src_iter = input.begin();
    output.resize(input.size() + 1);
    std::size_t decoded_length = 0;
    for (decoded_length = 0; src_iter != input.end(); ++decoded_length)
    {
        if (src_iter[0] == '%' && src_iter[1] && src_iter[2] && isxdigit(src_iter[1]) &&
            isxdigit(src_iter[2]))
        {
            std::string::value_type a = src_iter[1];
            std::string::value_type b = src_iter[2];
            a -= src_iter[1] < 58 ? 48 : src_iter[1] < 71 ? 55 : 87;
            b -= src_iter[2] < 58 ? 48 : src_iter[2] < 71 ? 55 : 87;
            output[decoded_length] = 16 * a + b;
            src_iter += 3;
            continue;
        }
        output[decoded_length] = *src_iter++;
    }
    output.resize(decoded_length);
    return decoded_length;
}

inline std::size_t URIDecodeInPlace(std::string &URI) { return URIDecode(URI, URI); }

// TODO: remove after switch to libosmium
inline bool StringStartsWith(const std::string &input, const std::string &prefix)
{
    return boost::starts_with(input, prefix);
}

inline std::string GetRandomString()
{
    std::string s;
    s.resize(128);
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < 127; ++i)
    {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[127] = 0;
    return s;
}

#endif // STRINGUTIL_H
