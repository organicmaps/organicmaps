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

#include "ScriptingEnvironment.h"

#include "ExtractionHelperFunctions.h"
#include "ExtractionWay.h"
#include "../DataStructures/ImportNode.h"
#include "../Util/LuaUtil.h"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../typedefs.h"

#include <sstream>

ScriptingEnvironment::ScriptingEnvironment() {}
ScriptingEnvironment::ScriptingEnvironment(const char *file_name)
: file_name(file_name)
{
    SimpleLogger().Write() << "Using script " << file_name;
}

void ScriptingEnvironment::initLuaState(lua_State* lua_state)
{
    luabind::open(lua_state);
    // open utility libraries string library;
    luaL_openlibs(lua_state);

    luaAddScriptFolderToLoadPath(lua_state, file_name.c_str());

    // Add our function to the state's global scope
    luabind::module(lua_state)[
        luabind::def("print", LUA_print<std::string>),
        luabind::def("durationIsValid", durationIsValid),
        luabind::def("parseDuration", parseDuration),

        luabind::class_<HashTable<std::string, std::string>>("keyVals")
        .def("Add", &HashTable<std::string, std::string>::Add)
        .def("Find", &HashTable<std::string, std::string>::Find)
        .def("Holds", &HashTable<std::string, std::string>::Holds),

        luabind::class_<ImportNode>("Node")
        // .def(luabind::constructor<>())
        .def_readwrite("lat", &ImportNode::lat)
        .def_readwrite("lon", &ImportNode::lon)
        .def_readonly("id", &ImportNode::node_id)
        .def_readwrite("bollard", &ImportNode::bollard)
        .def_readwrite("traffic_light", &ImportNode::trafficLight)
        .def_readwrite("tags", &ImportNode::keyVals),

       luabind::class_<ExtractionWay>("Way")
        // .def(luabind::constructor<>())
        .def_readonly("id", &ExtractionWay::id)
        .def_readwrite("name", &ExtractionWay::name)
        .def_readwrite("forward_speed", &ExtractionWay::forward_speed)
        .def_readwrite("backward_speed", &ExtractionWay::backward_speed)
        .def_readwrite("duration", &ExtractionWay::duration)
        .def_readwrite("access", &ExtractionWay::access)
        .def_readwrite("roundabout", &ExtractionWay::roundabout)
        .def_readwrite("is_access_restricted", &ExtractionWay::isAccessRestricted)
        .def_readwrite("ignore_in_grid", &ExtractionWay::ignoreInGrid)
        .def_readwrite("tags", &ExtractionWay::keyVals)
        .property("direction", &ExtractionWay::get_direction, &ExtractionWay::set_direction)
        .property("forward_mode", &ExtractionWay::get_forward_mode, &ExtractionWay::set_forward_mode)
        .property("backward_mode", &ExtractionWay::get_backward_mode, &ExtractionWay::set_backward_mode)
        .enum_("constants")[
            luabind::value("notSure", 0),
            luabind::value("oneway", 1),
            luabind::value("bidirectional", 2),
            luabind::value("opposite", 3)
        ],
        luabind::class_<std::vector<std::string>>("vector")
        .def("Add", static_cast<void (std::vector<std::string>::*)(const std::string &)>(&std::vector<std::string>::push_back))
    ];

    if (0 != luaL_dofile(lua_state, file_name.c_str()))
    {
        luabind::object error_msg(luabind::from_stack(lua_state, -1));
        std::ostringstream error_stream;
        error_stream << error_msg;
        throw OSRMException("ERROR occured in profile script:\n" + error_stream.str());
    }
}

lua_State *ScriptingEnvironment::getLuaState()
{
    bool initialized = false;
    auto& ref = script_contexts.local(initialized);
    if (!initialized)
    {
        std::shared_ptr<lua_State> state(luaL_newstate(), lua_close);
        ref = state;
        initLuaState(ref.get());
    }

    return ref.get();
}
