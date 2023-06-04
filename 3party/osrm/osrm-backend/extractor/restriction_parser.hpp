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

#ifndef RESTRICTION_PARSER_HPP
#define RESTRICTION_PARSER_HPP

#include "../data_structures/restriction.hpp"

#include <osmium/osm.hpp>
#include <osmium/tags/regex_filter.hpp>

#include <variant/optional.hpp>

#include <string>
#include <vector>

struct lua_State;
class ScriptingEnvironment;

class RestrictionParser
{
  public:
    // RestrictionParser(ScriptingEnvironment &scripting_environment);
    RestrictionParser(lua_State *lua_state);
    mapbox::util::optional<InputRestrictionContainer>
    TryParse(const osmium::Relation &relation) const;

  private:
    void ReadUseRestrictionsSetting(lua_State *lua_state);
    void ReadRestrictionExceptions(lua_State *lua_state);
    bool ShouldIgnoreRestriction(const std::string &except_tag_string) const;

    // lua_State *lua_state;
    std::vector<std::string> restriction_exceptions;
    bool use_turn_restrictions;
};

#endif /* RESTRICTION_PARSER_HPP */
