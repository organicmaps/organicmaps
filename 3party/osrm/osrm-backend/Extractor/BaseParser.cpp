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

#include "BaseParser.h"
#include "ExtractionWay.h"
#include "ScriptingEnvironment.h"

#include "../DataStructures/ImportNode.h"
#include "../Util/LuaUtil.h"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/ref.hpp>
#include <boost/regex.hpp>

BaseParser::BaseParser(ExtractorCallbacks *extractor_callbacks,
                       ScriptingEnvironment &scripting_environment)
    : extractor_callbacks(extractor_callbacks),
      lua_state(scripting_environment.getLuaState()),
      scripting_environment(scripting_environment), use_turn_restrictions(true)
{
    ReadUseRestrictionsSetting();
    ReadRestrictionExceptions();
}

void BaseParser::ReadUseRestrictionsSetting()
{
    if (0 != luaL_dostring(lua_state, "return use_turn_restrictions\n"))
    {
        use_turn_restrictions = false;
    }
    else if (lua_isboolean(lua_state, -1))
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

void BaseParser::ReadRestrictionExceptions()
{
    if (lua_function_exists(lua_state, "get_exceptions"))
    {
        // get list of turn restriction exceptions
        luabind::call_function<void>(
            lua_state, "get_exceptions", boost::ref(restriction_exceptions));
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

void BaseParser::report_errors(lua_State *lua_state, const int status) const
{
    if (0 != status)
    {
        std::cerr << "-- " << lua_tostring(lua_state, -1) << std::endl;
        lua_pop(lua_state, 1); // remove error message
    }
}

void BaseParser::ParseNodeInLua(ImportNode &node, lua_State *local_lua_state)
{
    luabind::call_function<void>(local_lua_state, "node_function", boost::ref(node));
}

void BaseParser::ParseWayInLua(ExtractionWay &way, lua_State *local_lua_state)
{
    luabind::call_function<void>(local_lua_state, "way_function", boost::ref(way));
}

bool BaseParser::ShouldIgnoreRestriction(const std::string &except_tag_string) const
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
    for (std::string &current_string : exceptions)
    {
        const auto string_iterator =
            std::find(restriction_exceptions.begin(), restriction_exceptions.end(), current_string);
        if (restriction_exceptions.end() != string_iterator)
        {
            return true;
        }
    }
    return false;
}
