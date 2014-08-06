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

#ifndef LUA_UTIL_H
#define LUA_UTIL_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <boost/filesystem/convenience.hpp>
#include <luabind/luabind.hpp>

#include <iostream>
#include <string>

template <typename T> void LUA_print(T output) { std::cout << "[LUA] " << output << std::endl; }

// Check if the lua function <name> is defined
inline bool lua_function_exists(lua_State *lua_state, const char *name)
{
    luabind::object globals_table = luabind::globals(lua_state);
    luabind::object lua_function = globals_table[name];
    return lua_function && (luabind::type(lua_function) == LUA_TFUNCTION);
}

// Add the folder contain the script to the lua load path, so script can easily require() other lua
// scripts inside that folder, or subfolders.
// See http://lua-users.org/wiki/PackagePath for details on the package.path syntax.
inline void luaAddScriptFolderToLoadPath(lua_State *lua_state, const char *file_name)
{
    const boost::filesystem::path profile_path(file_name);
    std::string folder = profile_path.parent_path().string();
    // TODO: This code is most probably not Windows safe since it uses UNIX'ish path delimiters
    const std::string lua_code =
        "package.path = \"" + folder + "/?.lua;profiles/?.lua;\" .. package.path";
    luaL_dostring(lua_state, lua_code.c_str());
}

#endif // LUA_UTIL_H
