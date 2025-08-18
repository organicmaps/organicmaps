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

#ifndef EXTRACTOR_CALLBACKS_HPP
#define EXTRACTOR_CALLBACKS_HPP

#include "extraction_way.hpp"
#include "../typedefs.h"

#include <osmium/osm.hpp>

#include <variant/optional.hpp>

#include <string>
#include <unordered_map>

struct ExternalMemoryNode;
class ExtractionContainers;
struct InputRestrictionContainer;
struct ExtractionNode;

class ExtractorCallbacks
{
  private:
    std::unordered_map<std::string, NodeID> &string_map;
    ExtractionContainers &external_memory;

  public:
    ExtractorCallbacks() = delete;
    ExtractorCallbacks(const ExtractorCallbacks &) = delete;
    explicit ExtractorCallbacks(ExtractionContainers &extraction_containers,
                                std::unordered_map<std::string, NodeID> &string_map);

    // warning: caller needs to take care of synchronization!
    void ProcessNode(const osmium::Node &current_node, const ExtractionNode &result_node);

    // warning: caller needs to take care of synchronization!
    void ProcessRestriction(const mapbox::util::optional<InputRestrictionContainer> &restriction);

    // warning: caller needs to take care of synchronization!
    void ProcessWay(const osmium::Way &current_way, const ExtractionWay &result_way);
};

#endif /* EXTRACTOR_CALLBACKS_HPP */
