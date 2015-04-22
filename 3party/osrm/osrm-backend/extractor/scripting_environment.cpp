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

#include "scripting_environment.hpp"

#include "extraction_helper_functions.hpp"
#include "extraction_node.hpp"
#include "extraction_way.hpp"
#include "../data_structures/external_memory_node.hpp"
#include "../util/lua_util.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"
#include "../typedefs.h"

#include <luabind/tag_function.hpp>

#include <osmium/osm.hpp>

#include <sstream>
namespace
{
// wrapper method as luabind doesn't automatically overload funcs w/ default parameters
template <class T>
auto get_value_by_key(T const &object, const char *key) -> decltype(object.get_value_by_key(key))
{
    return object.get_value_by_key(key, "");
}

int lua_error_callback(lua_State *L) // This is so I can use my own function as an
// exception handler, pcall_log()
{
    luabind::object error_msg(luabind::from_stack(L, -1));
    std::ostringstream error_stream;
    error_stream << error_msg;
    throw osrm::exception("ERROR occured in profile script:\n" + error_stream.str());
}
}

ScriptingEnvironment::ScriptingEnvironment(const std::string &file_name) : file_name(file_name)
{
    SimpleLogger().Write() << "Using script " << file_name;
}

void ScriptingEnvironment::init_lua_state(lua_State *lua_state)
{
    typedef double (osmium::Location::*location_member_ptr_type)() const;

    luabind::open(lua_state);
    // open utility libraries string library;
    luaL_openlibs(lua_state);

    luaAddScriptFolderToLoadPath(lua_state, file_name.c_str());

    // Add our function to the state's global scope
    luabind::module(lua_state)[
        luabind::def("print", LUA_print<std::string>),
        luabind::def("durationIsValid", durationIsValid),
        luabind::def("parseDuration", parseDuration),

        luabind::class_<std::vector<std::string>>("vector")
            .def("Add", static_cast<void (std::vector<std::string>::*)(const std::string &)>(
                            &std::vector<std::string>::push_back)),

        luabind::class_<osmium::Location>("Location")
            .def<location_member_ptr_type>("lat", &osmium::Location::lat)
            .def<location_member_ptr_type>("lon", &osmium::Location::lon),

        luabind::class_<osmium::Node>("Node")
            // .def<node_member_ptr_type>("tags", &osmium::Node::tags)
            .def("location", &osmium::Node::location)
            .def("get_value_by_key", &osmium::Node::get_value_by_key)
            .def("get_value_by_key", &get_value_by_key<osmium::Node>)
            .def("id", &osmium::Node::id),

        luabind::class_<ExtractionNode>("ResultNode")
            .def_readwrite("traffic_lights", &ExtractionNode::traffic_lights)
            .def_readwrite("barrier", &ExtractionNode::barrier),

        luabind::class_<ExtractionWay>("ResultWay")
            // .def(luabind::constructor<>())
            .def_readwrite("forward_speed", &ExtractionWay::forward_speed)
            .def_readwrite("backward_speed", &ExtractionWay::backward_speed)
            .def_readwrite("name", &ExtractionWay::name)
            .def_readwrite("roundabout", &ExtractionWay::roundabout)
            .def_readwrite("is_access_restricted", &ExtractionWay::is_access_restricted)
            .def_readwrite("ignore_in_index", &ExtractionWay::ignore_in_grid)
            .def_readwrite("duration", &ExtractionWay::duration)
            .property("forward_mode", &ExtractionWay::get_forward_mode,
                      &ExtractionWay::set_forward_mode)
            .property("backward_mode", &ExtractionWay::get_backward_mode,
                      &ExtractionWay::set_backward_mode)
            .enum_("constants")[
                luabind::value("notSure", 0),
                luabind::value("oneway", 1),
                luabind::value("bidirectional", 2),
                luabind::value("opposite", 3)
            ],
        luabind::class_<osmium::Way>("Way")
            .def("get_value_by_key", &osmium::Way::get_value_by_key)
            .def("get_value_by_key", &get_value_by_key<osmium::Way>)
            .def("id", &osmium::Way::id)
    ];

    if (0 != luaL_dofile(lua_state, file_name.c_str()))
    {
        luabind::object error_msg(luabind::from_stack(lua_state, -1));
        std::ostringstream error_stream;
        error_stream << error_msg;
        throw osrm::exception("ERROR occured in profile script:\n" + error_stream.str());
    }
}

lua_State *ScriptingEnvironment::get_lua_state()
{
    std::lock_guard<std::mutex> lock(init_mutex);
    bool initialized = false;
    auto &ref = script_contexts.local(initialized);
    if (!initialized)
    {
        std::shared_ptr<lua_State> state(luaL_newstate(), lua_close);
        ref = state;
        init_lua_state(ref.get());
    }
    luabind::set_pcall_callback(&lua_error_callback);

    return ref.get();
}
