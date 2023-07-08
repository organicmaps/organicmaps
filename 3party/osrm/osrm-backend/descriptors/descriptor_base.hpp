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

#ifndef DESCRIPTOR_BASE_HPP
#define DESCRIPTOR_BASE_HPP

#include "../data_structures/coordinate_calculation.hpp"
#include "../data_structures/internal_route_result.hpp"
#include "../data_structures/phantom_node.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <osrm/json_container.hpp>

#include <string>
#include <unordered_map>
#include <vector>

struct DescriptorTable : public std::unordered_map<std::string, unsigned>
{
    unsigned get_id(const std::string &key)
    {
        auto iter = find(key);
        if (iter != end())
        {
            return iter->second;
        }
        return 0;
    }
};

struct DescriptorConfig
{
    DescriptorConfig() : instructions(true), geometry(true), encode_geometry(true), zoom_level(18)
    {
    }

    template <class OtherT>
    DescriptorConfig(const OtherT &other)
        : instructions(other.print_instructions), geometry(other.geometry),
          encode_geometry(other.compression), zoom_level(other.zoom_level)
    {
        BOOST_ASSERT(zoom_level >= 0);
    }

    bool instructions;
    bool geometry;
    bool encode_geometry;
    short zoom_level;
};

template <class DataFacadeT> class BaseDescriptor
{
  public:
    BaseDescriptor() {}
    // Maybe someone can explain the pure virtual destructor thing to me (dennis)
    virtual ~BaseDescriptor() {}
    virtual void Run(const InternalRouteResult &raw_route, osrm::json::Object &json_result) = 0;
    virtual void SetConfig(const DescriptorConfig &c) = 0;
};

#endif // DESCRIPTOR_BASE_HPP
