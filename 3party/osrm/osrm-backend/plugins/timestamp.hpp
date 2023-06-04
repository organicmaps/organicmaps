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

#ifndef TIMESTAMP_PLUGIN_H
#define TIMESTAMP_PLUGIN_H

#include "plugin_base.hpp"

#include "../util/json_renderer.hpp"

#include <osrm/json_container.hpp>

#include <string>

template <class DataFacadeT> class TimestampPlugin final : public BasePlugin
{
  public:
    explicit TimestampPlugin(const DataFacadeT *facade)
        : facade(facade), descriptor_string("timestamp")
    {
    }
    const std::string GetDescriptor() const override final { return descriptor_string; }
    int HandleRequest(const RouteParameters &route_parameters,
                      osrm::json::Object &json_result) override final
    {
        json_result.values["status"] = 0;
        const std::string timestamp = facade->GetTimestamp();
        json_result.values["timestamp"] = timestamp;
        return 200;
    }

  private:
    const DataFacadeT *facade;
    std::string descriptor_string;
};

#endif /* TIMESTAMP_PLUGIN_H */
