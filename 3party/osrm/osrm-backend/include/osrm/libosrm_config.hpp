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

#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <osrm/server_paths.hpp>

struct libosrm_config
{
    libosrm_config(const libosrm_config &) = delete;
    libosrm_config()
        : max_locations_distance_table(100), max_locations_map_matching(-1),
          use_shared_memory(false)
    {
    }

    libosrm_config(const ServerPaths &paths, const bool flag, const int max_table, const int max_matching)
        : server_paths(paths), max_locations_distance_table(max_table),
          max_locations_map_matching(max_matching), use_shared_memory(flag)
    {
    }

    ServerPaths server_paths;
    int max_locations_distance_table;
    int max_locations_map_matching;
    bool use_shared_memory;
};

#endif // SERVER_CONFIG_HPP
