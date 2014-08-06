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

// based on https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_CONTAINER_H
#define JSON_CONTAINER_H

#include "../Util/StringUtil.h"

#include <boost/variant.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

namespace JSON
{

struct String;
struct Number;
struct Object;
struct Array;
struct True;
struct False;
struct Null;

typedef boost::variant<boost::recursive_wrapper<String>,
                       boost::recursive_wrapper<Number>,
                       boost::recursive_wrapper<Object>,
                       boost::recursive_wrapper<Array>,
                       boost::recursive_wrapper<True>,
                       boost::recursive_wrapper<False>,
                       boost::recursive_wrapper<Null> > Value;

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
    Number(double value) : value(value) {}
    double value;
};

struct Object
{
    std::unordered_map<std::string, Value> values;
};

struct Array
{
    std::vector<Value> values;
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

struct Renderer : boost::static_visitor<>
{
    Renderer(std::ostream &_out) : out(_out) {}

    void operator()(const String &string) const { out << "\"" << string.value << "\""; }

    void operator()(const Number &number) const
    {
        out.precision(10);
        out << number.value;
    }

    void operator()(const Object &object) const
    {
        out << "{";
        auto iterator = object.values.begin();
        while (iterator != object.values.end())
        {
            out << "\"" << (*iterator).first << "\":";
            boost::apply_visitor(Renderer(out), (*iterator).second);
            if (++iterator != object.values.end())
            {
                out << ",";
            }
        }
        out << "}";
    }

    void operator()(const Array &array) const
    {
        out << "[";
        std::vector<Value>::const_iterator iterator;
        iterator = array.values.begin();
        while (iterator != array.values.end())
        {
            boost::apply_visitor(Renderer(out), *iterator);
            if (++iterator != array.values.end())
            {
                out << ",";
            }
        }
        out << "]";
    }

    void operator()(const True &) const { out << "true"; }

    void operator()(const False &) const { out << "false"; }

    void operator()(const Null &) const { out << "null"; }

  private:
    std::ostream &out;
};

struct ArrayRenderer : boost::static_visitor<>
{
    ArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const String &string) const {
        out.push_back('\"');
        out.insert(out.end(), string.value.begin(), string.value.end());
        out.push_back('\"');
    }

    void operator()(const Number &number) const
    {
        const std::string number_string = FixedDoubleToString(number.value);
        out.insert(out.end(), number_string.begin(), number_string.end());
    }

    void operator()(const Object &object) const
    {
        out.push_back('{');
        auto iterator = object.values.begin();
        while (iterator != object.values.end())
        {
            out.push_back('\"');
            out.insert(out.end(), (*iterator).first.begin(), (*iterator).first.end());
            out.push_back('\"');
            out.push_back(':');

            boost::apply_visitor(ArrayRenderer(out), (*iterator).second);
            if (++iterator != object.values.end())
            {
                out.push_back(',');
            }
        }
        out.push_back('}');
    }

    void operator()(const Array &array) const
    {
        out.push_back('[');
        std::vector<Value>::const_iterator iterator;
        iterator = array.values.begin();
        while (iterator != array.values.end())
        {
            boost::apply_visitor(ArrayRenderer(out), *iterator);
            if (++iterator != array.values.end())
            {
                out.push_back(',');
            }
        }
        out.push_back(']');
    }

    void operator()(const True &) const {
        const std::string temp("true");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const False &) const {
        const std::string temp("false");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const Null &) const {
        const std::string temp("null");
        out.insert(out.end(), temp.begin(), temp.end());
    }

  private:
    std::vector<char> &out;
};

inline void render(std::ostream &out, const Object &object)
{
    Value value = object;
    boost::apply_visitor(Renderer(out), value);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Value value = object;
    boost::apply_visitor(ArrayRenderer(out), value);
}

} // namespace JSON

#endif // JSON_CONTAINER_H
