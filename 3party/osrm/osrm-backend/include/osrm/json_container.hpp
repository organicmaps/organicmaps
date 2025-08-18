/*

Copyright (c) 2013, Project OSRM contributors
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

// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_CONTAINER_HPP
#define JSON_CONTAINER_HPP

#include <variant/variant.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

namespace osrm
{
namespace json
{

struct Object;
struct Array;

struct String
{
    String() {}
    String(const char *value) : value(value) {}
    String(const std::string &value) : value(value) {}
    std::string value;
};

struct Number
{
    Number() {}
    Number(double value) : value(static_cast<double>(value)) {}
    double value;
};

struct True
{
};

struct False
{
};

struct Null
{
};

using Value = mapbox::util::variant<String,
                                    Number,
                                    mapbox::util::recursive_wrapper<Object>,
                                    mapbox::util::recursive_wrapper<Array>,
                                    True,
                                    False,
                                    Null>;

struct Object
{
    std::unordered_map<std::string, Value> values;
};

struct Array
{
    std::vector<Value> values;
};

} // namespace JSON
} // namespace osrm
#endif // JSON_CONTAINER_HPP
