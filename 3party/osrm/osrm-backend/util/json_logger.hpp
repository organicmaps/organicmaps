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

#ifndef JSON_LOGGER_HPP
#define JSON_LOGGER_HPP

#include <osrm/json_container.hpp>

#include <boost/thread.hpp>

namespace osrm
{
namespace json
{

// Used to append additional debugging information to the JSON response in a
// thread safe manner.
class Logger
{
    using MapT = std::unordered_map<std::string, osrm::json::Value>;

  public:
    static Logger* get()
    {
        static Logger logger;

        bool return_logger = true;
#ifdef NDEBUG
        return_logger = false;
#endif
#ifdef ENABLE_JSON_LOGGING
        return_logger = true;
#endif

        if (return_logger)
        {
            return &logger;
        }

        return nullptr;
    }

    void initialize(const std::string& name)
    {
        if (!map.get())
        {
            map.reset(new MapT());
        }
        (*map)[name] = Object();
    }

    void render(const std::string& name, Object& obj) const
    {
        obj.values["debug"] = map->at(name);
    }

    boost::thread_specific_ptr<MapT> map;
};


}
}

#endif /* JSON_LOGGER_HPP */
