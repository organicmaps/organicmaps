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

#include "../library/osrm.hpp"
#include "../util/git_sha.hpp"
#include "../util/json_renderer.hpp"
#include "../util/routed_options.hpp"
#include "../util/simple_logger.hpp"

#include <osrm/json_container.hpp>
#include <osrm/libosrm_config.hpp>
#include <osrm/route_parameters.hpp>

#include <string>

int main(int argc, const char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        std::string ip_address;
        int ip_port, requested_thread_num, max_locations_map_matching;
        bool trial_run = false;
        libosrm_config lib_config;
        const unsigned init_result = GenerateServerProgramOptions(
            argc, argv, lib_config.server_paths, ip_address, ip_port, requested_thread_num,
            lib_config.use_shared_memory, trial_run, lib_config.max_locations_distance_table,
            max_locations_map_matching);

        if (init_result == INIT_OK_DO_NOT_START_ENGINE)
        {
            return 0;
        }
        if (init_result == INIT_FAILED)
        {
            return 1;
        }
        SimpleLogger().Write() << "starting up engines, " << g_GIT_DESCRIPTION;

        OSRM routing_machine(lib_config);

        RouteParameters route_parameters;
        route_parameters.zoom_level = 18;           // no generalization
        route_parameters.print_instructions = true; // turn by turn instructions
        route_parameters.alternate_route = true;    // get an alternate route, too
        route_parameters.geometry = true;           // retrieve geometry of route
        route_parameters.compression = true;        // polyline encoding
        route_parameters.check_sum = -1;            // see wiki
        route_parameters.service = "viaroute";      // that's routing
        route_parameters.output_format = "json";
        route_parameters.jsonp_parameter = ""; // set for jsonp wrapping
        route_parameters.language = "";        // unused atm
        // route_parameters.hints.push_back(); // see wiki, saves I/O if done properly

        // start_coordinate
        route_parameters.coordinates.emplace_back(52.519930 * COORDINATE_PRECISION,
                                                  13.438640 * COORDINATE_PRECISION);
        // target_coordinate
        route_parameters.coordinates.emplace_back(52.513191 * COORDINATE_PRECISION,
                                                  13.415852 * COORDINATE_PRECISION);
        osrm::json::Object json_result;
        const int result_code = routing_machine.RunQuery(route_parameters, json_result);
        SimpleLogger().Write() << "http code: " << result_code;
        osrm::json::render(SimpleLogger().Write(), json_result);
    }
    catch (std::exception &current_exception)
    {
        SimpleLogger().Write(logWARNING) << "caught exception: " << current_exception.what();
        return -1;
    }
    return 0;
}
