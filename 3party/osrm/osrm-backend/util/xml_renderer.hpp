/*

Copyright (c) 2014, Project OSRM contributors
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

#ifndef XML_RENDERER_HPP
#define XML_RENDERER_HPP

#include "cast.hpp"

#include <osrm/json_container.hpp>

namespace osrm
{
namespace json
{

struct XMLToArrayRenderer : mapbox::util::static_visitor<>
{
    explicit XMLToArrayRenderer(std::vector<char> &_out) : out(_out) {}

    void operator()(const String &string) const
    {
        out.push_back('\"');
        out.insert(out.end(), string.value.begin(), string.value.end());
        out.push_back('\"');
    }

    void operator()(const Number &number) const
    {
        const std::string number_string = cast::double_fixed_to_string(number.value);
        out.insert(out.end(), number_string.begin(), number_string.end());
    }

    void operator()(const Object &object) const
    {
        auto iterator = object.values.begin();
        while (iterator != object.values.end())
        {
            if (iterator->first.at(0) != '_')
            {
                out.push_back('<');
                out.insert(out.end(), (*iterator).first.begin(), (*iterator).first.end());
            }
            else
            {
                out.push_back(' ');
                out.insert(out.end(), ++(*iterator).first.begin(), (*iterator).first.end());
                out.push_back('=');
            }
            mapbox::util::apply_visitor(XMLToArrayRenderer(out), (*iterator).second);
            if (iterator->first.at(0) != '_')
            {
                out.push_back('/');
                out.push_back('>');
            }
            ++iterator;
        }
    }

    void operator()(const Array &array) const
    {
        std::vector<Value>::const_iterator iterator;
        iterator = array.values.begin();
        while (iterator != array.values.end())
        {
            mapbox::util::apply_visitor(XMLToArrayRenderer(out), *iterator);
            ++iterator;
        }
    }

    void operator()(const True &) const
    {
        const std::string temp("true");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const False &) const
    {
        const std::string temp("false");
        out.insert(out.end(), temp.begin(), temp.end());
    }

    void operator()(const Null &) const
    {
        const std::string temp("null");
        out.insert(out.end(), temp.begin(), temp.end());
    }

  private:
    std::vector<char> &out;
};

template <class JSONObject> inline void xml_render(std::vector<char> &out, const JSONObject &object)
{
    Value value = object;
    mapbox::util::apply_visitor(XMLToArrayRenderer(out), value);
}

template <class JSONObject> inline void gpx_render(std::vector<char> &out, const JSONObject &object)
{
    // add header

    const std::string header{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx creator=\"OSRM Routing Engine\""
        " version=\"1.1\" xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:xsi=\"http:"
        "//www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topogr"
        "afix.com/GPX/1/1 gpx.xsd\"><metadata><copyright author=\"Project OSRM\"><lice"
        "nse>Data (c) OpenStreetMap contributors (ODbL)</license></copyright></metadat"
        "a><rte>"};
    out.insert(out.end(), header.begin(), header.end());

    xml_render(out, object);

    const std::string footer{"</rte></gpx>"};
    out.insert(out.end(), footer.begin(), footer.end());
}
} // namespace json
} // namespace osrm
#endif // XML_RENDERER_HPP
