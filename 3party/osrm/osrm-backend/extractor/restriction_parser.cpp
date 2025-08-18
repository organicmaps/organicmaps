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

#include "restriction_parser.hpp"
#include "extraction_way.hpp"
#include "scripting_environment.hpp"

#include "../data_structures/external_memory_node.hpp"
#include "../util/lua_util.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/ref.hpp>
#include <boost/regex.hpp>

#include <algorithm>

namespace
{
int lua_error_callback(lua_State *lua_state)
{
    luabind::object error_msg(luabind::from_stack(lua_state, -1));
    std::ostringstream error_stream;
    error_stream << error_msg;
    throw osrm::exception("ERROR occured in profile script:\n" + error_stream.str());
}
}

RestrictionParser::RestrictionParser(lua_State *lua_state)
    : /*lua_state(scripting_environment.getLuaState()),*/ use_turn_restrictions(true)
{
    ReadUseRestrictionsSetting(lua_state);

    if (use_turn_restrictions)
    {
        ReadRestrictionExceptions(lua_state);
    }
}

void RestrictionParser::ReadUseRestrictionsSetting(lua_State *lua_state)
{
    if (0 == luaL_dostring(lua_state, "return use_turn_restrictions\n") &&
        lua_isboolean(lua_state, -1))
    {
        use_turn_restrictions = lua_toboolean(lua_state, -1);
    }

    if (use_turn_restrictions)
    {
        SimpleLogger().Write() << "Using turn restrictions";
    }
    else
    {
        SimpleLogger().Write() << "Ignoring turn restrictions";
    }
}

void RestrictionParser::ReadRestrictionExceptions(lua_State *lua_state)
{
    if (lua_function_exists(lua_state, "get_exceptions"))
    {
        luabind::set_pcall_callback(&lua_error_callback);
        // get list of turn restriction exceptions
        luabind::call_function<void>(lua_state, "get_exceptions",
                                     boost::ref(restriction_exceptions));
        const unsigned exception_count = restriction_exceptions.size();
        SimpleLogger().Write() << "Found " << exception_count
                               << " exceptions to turn restrictions:";
        for (const std::string &str : restriction_exceptions)
        {
            SimpleLogger().Write() << "  " << str;
        }
    }
    else
    {
        SimpleLogger().Write() << "Found no exceptions to turn restrictions";
    }
}

mapbox::util::optional<InputRestrictionContainer>
RestrictionParser::TryParse(const osmium::Relation &relation) const
{
    // return if turn restrictions should be ignored
    if (!use_turn_restrictions)
    {
        return mapbox::util::optional<InputRestrictionContainer>();
    }

    osmium::tags::KeyPrefixFilter filter(false);
    filter.add(true, "restriction");

    const osmium::TagList &tag_list = relation.tags();

    osmium::tags::KeyPrefixFilter::iterator fi_begin(filter, tag_list.begin(), tag_list.end());
    osmium::tags::KeyPrefixFilter::iterator fi_end(filter, tag_list.end(), tag_list.end());

    // if it's a restriction, continue;
    if (std::distance(fi_begin, fi_end) == 0)
    {
        return mapbox::util::optional<InputRestrictionContainer>();
    }

    // check if the restriction should be ignored
    const char *except = relation.get_value_by_key("except");
    if (except != nullptr && ShouldIgnoreRestriction(except))
    {
        return mapbox::util::optional<InputRestrictionContainer>();
    }

    bool is_only_restriction = false;

    for (auto iter = fi_begin; iter != fi_end; ++iter)
    {
        if (std::string("restriction") == iter->key() ||
            std::string("restriction::hgv") == iter->key())
        {
            const std::string restriction_value(iter->value());

            if (restriction_value.find("only_") == 0)
            {
                is_only_restriction = true;
            }
        }
    }

    InputRestrictionContainer restriction_container(is_only_restriction);

    for (const auto &member : relation.members())
    {
        const char *role = member.role();
        if (strcmp("from", role) != 0 && strcmp("to", role) != 0 && strcmp("via", role) != 0)
        {
            continue;
        }

        switch (member.type())
        {
        case osmium::item_type::node:
            // Make sure nodes appear only in the role if a via node
            if (0 == strcmp("from", role) || 0 == strcmp("to", role))
            {
                continue;
            }
            BOOST_ASSERT(0 == strcmp("via", role));

            // set via node id
            restriction_container.restriction.via.node = member.ref();
            break;

        case osmium::item_type::way:
            BOOST_ASSERT(0 == strcmp("from", role) || 0 == strcmp("to", role) ||
                         0 == strcmp("via", role));
            if (0 == strcmp("from", role))
            {
                restriction_container.restriction.from.way = member.ref();
            }
            else if (0 == strcmp("to", role))
            {
                restriction_container.restriction.to.way = member.ref();
            }
            // else if (0 == strcmp("via", role))
            // {
            //     not yet suppported
            //     restriction_container.restriction.via.way = member.ref();
            // }
            break;
        case osmium::item_type::relation:
            // not yet supported, but who knows what the future holds...
            break;
        default:
            // shouldn't ever happen
            break;
        }
    }
    return mapbox::util::optional<InputRestrictionContainer>(restriction_container);
}

bool RestrictionParser::ShouldIgnoreRestriction(const std::string &except_tag_string) const
{
    // should this restriction be ignored? yes if there's an overlap between:
    // a) the list of modes in the except tag of the restriction
    //    (except_tag_string), eg: except=bus;bicycle
    // b) the lua profile defines a hierachy of modes,
    //    eg: [access, vehicle, bicycle]

    if (except_tag_string.empty())
    {
        return false;
    }

    // Be warned, this is quadratic work here, but we assume that
    // only a few exceptions are actually defined.
    std::vector<std::string> exceptions;
    boost::algorithm::split_regex(exceptions, except_tag_string, boost::regex("[;][ ]*"));

    return std::any_of(std::begin(exceptions), std::end(exceptions),
                       [&](const std::string &current_string)
                       {
                           if (std::end(restriction_exceptions) !=
                               std::find(std::begin(restriction_exceptions),
                                         std::end(restriction_exceptions), current_string))
                           {
                               return true;
                           }
                           return false;
                       });
}
