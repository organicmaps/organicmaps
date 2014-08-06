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

#ifndef BASEPARSER_H_
#define BASEPARSER_H_

#include <string>
#include <vector>

struct lua_State;
class ExtractorCallbacks;
class ScriptingEnvironment;
struct ExtractionWay;
struct ImportNode;

class BaseParser
{
  public:
    BaseParser() = delete;
    BaseParser(const BaseParser &) = delete;
    BaseParser(ExtractorCallbacks *extractor_callbacks,
               ScriptingEnvironment &scripting_environment);
    virtual ~BaseParser() {}
    virtual bool ReadHeader() = 0;
    virtual bool Parse() = 0;

    virtual void ParseNodeInLua(ImportNode &node, lua_State *lua_state);
    virtual void ParseWayInLua(ExtractionWay &way, lua_State *lua_state);
    virtual void report_errors(lua_State *lua_state, const int status) const;

  protected:
    virtual void ReadUseRestrictionsSetting();
    virtual void ReadRestrictionExceptions();
    virtual bool ShouldIgnoreRestriction(const std::string &except_tag_string) const;

    ExtractorCallbacks *extractor_callbacks;
    lua_State *lua_state;
    ScriptingEnvironment &scripting_environment;
    std::vector<std::string> restriction_exceptions;
    bool use_turn_restrictions;
};

#endif /* BASEPARSER_H_ */
