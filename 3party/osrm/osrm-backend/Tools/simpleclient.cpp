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

#include "../Library/OSRM.h"
#include "../Util/GitDescription.h"
#include "../Util/ProgramOptions.h"
#include "../Util/simple_logger.hpp"

#include <osrm/Reply.h>
#include <osrm/RouteParameters.h>
#include <osrm/ServerPaths.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <stack>
#include <string>
#include <sstream>

// Dude, real recursions on the OS stack? You must be brave...
void print_tree(boost::property_tree::ptree const &property_tree, const unsigned recursion_depth)
{
    auto end = property_tree.end();
    for (auto tree_iterator = property_tree.begin(); tree_iterator != end; ++tree_iterator)
    {
        for (unsigned current_recursion = 0; current_recursion < recursion_depth;
             ++current_recursion)
        {
            std::cout << " " << std::flush;
        }
        std::cout << tree_iterator->first << ": " << tree_iterator->second.get_value<std::string>()
                  << std::endl;
        print_tree(tree_iterator->second, recursion_depth + 1);
    }
}

int main(int argc, const char *argv[])
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        std::string ip_address;
        int ip_port, requested_thread_num;
        bool use_shared_memory = false, trial = false;
        ServerPaths server_paths;
        if (!GenerateServerProgramOptions(argc,
                                          argv,
                                          server_paths,
                                          ip_address,
                                          ip_port,
                                          requested_thread_num,
                                          use_shared_memory,
                                          trial))
        {
            return 0;
        }

        SimpleLogger().Write() << "starting up engines, " << g_GIT_DESCRIPTION;

        OSRM routing_machine(server_paths, use_shared_memory);

        RouteParameters route_parameters;
        route_parameters.zoom_level = 18;           // no generalization
        route_parameters.print_instructions = true; // turn by turn instructions
        route_parameters.alternate_route = true;    // get an alternate route, too
        route_parameters.geometry = true;           // retrieve geometry of route
        route_parameters.compression = true;        // polyline encoding
        route_parameters.check_sum = UINT_MAX;      // see wiki
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
        http::Reply osrm_reply;
        routing_machine.RunQuery(route_parameters, osrm_reply);

        // attention: super-inefficient hack below:

        std::stringstream my_stream;
        for (const auto &element : osrm_reply.content)
        {
            std::cout << element;
            my_stream << element;
        }
        std::cout << std::endl;

        boost::property_tree::ptree property_tree;
        boost::property_tree::read_json(my_stream, property_tree);

        print_tree(property_tree, 0);
    }
    catch (std::exception &current_exception)
    {
        SimpleLogger().Write(logWARNING) << "caught exception: " << current_exception.what();
        return -1;
    }
    return 0;
}
