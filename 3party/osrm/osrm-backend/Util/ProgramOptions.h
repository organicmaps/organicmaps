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

#ifndef PROGAM_OPTIONS_H
#define PROGAM_OPTIONS_H

#include "GitDescription.h"
#include "IniFileUtil.h"
#include "OSRMException.h"
#include "simple_logger.hpp"

#include <osrm/ServerPaths.h>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <string>

const static unsigned INIT_OK_START_ENGINE = 0;
const static unsigned INIT_OK_DO_NOT_START_ENGINE = 1;
const static unsigned INIT_FAILED = -1;

inline void populate_base_path(ServerPaths &server_paths)
{
    // populate the server_path object
    auto path_iterator = server_paths.find("base");

    // if a base path has been set, we populate it.
    if (path_iterator != server_paths.end())
    {
        const std::string base_string = path_iterator->second.string();
        SimpleLogger().Write() << "populating base path: " << base_string;

        server_paths["hsgrdata"] = base_string + ".hsgr";
        BOOST_ASSERT(server_paths.find("hsgrdata") != server_paths.end());
        server_paths["nodesdata"] = base_string + ".nodes";
        BOOST_ASSERT(server_paths.find("nodesdata") != server_paths.end());
        server_paths["edgesdata"] = base_string + ".edges";
        BOOST_ASSERT(server_paths.find("edgesdata") != server_paths.end());
        server_paths["geometries"] = base_string + ".geometry";
        BOOST_ASSERT(server_paths.find("geometries") != server_paths.end());
        server_paths["ramindex"] = base_string + ".ramIndex";
        BOOST_ASSERT(server_paths.find("ramindex") != server_paths.end());
        server_paths["fileindex"] = base_string + ".fileIndex";
        BOOST_ASSERT(server_paths.find("fileindex") != server_paths.end());
        server_paths["namesdata"] = base_string + ".names";
        BOOST_ASSERT(server_paths.find("namesdata") != server_paths.end());
        server_paths["timestamp"] = base_string + ".timestamp";
        BOOST_ASSERT(server_paths.find("timestamp") != server_paths.end());
    }

    // check if files are give and whether they exist at all
    path_iterator = server_paths.find("hsgrdata");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        if (path_iterator == server_paths.end())
        {
            SimpleLogger().Write() << "hsgrdata unset";
        }
        if (!boost::filesystem::is_regular_file(path_iterator->second))
        {
            SimpleLogger().Write() << "not a regular file";
        }

        throw OSRMException(".hsgr not found: " + path_iterator->second.string());
    }

    path_iterator = server_paths.find("nodesdata");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".nodes not found");
    }

    path_iterator = server_paths.find("edgesdata");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".edges not found");
    }

    path_iterator = server_paths.find("geometries");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".geometry not found");
    }

    path_iterator = server_paths.find("ramindex");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".ramIndex not found");
    }

    path_iterator = server_paths.find("fileindex");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".fileIndex not found");
    }

    path_iterator = server_paths.find("namesdata");
    if (path_iterator == server_paths.end() ||
        !boost::filesystem::is_regular_file(path_iterator->second))
    {
        throw OSRMException(".namesIndex not found");
    }

    SimpleLogger().Write() << "HSGR file:\t" << server_paths["hsgrdata"];
    SimpleLogger().Write(logDEBUG) << "Nodes file:\t" << server_paths["nodesdata"];
    SimpleLogger().Write(logDEBUG) << "Edges file:\t" << server_paths["edgesdata"];
    SimpleLogger().Write(logDEBUG) << "Geometry file:\t" << server_paths["geometries"];
    SimpleLogger().Write(logDEBUG) << "RAM file:\t" << server_paths["ramindex"];
    SimpleLogger().Write(logDEBUG) << "Index file:\t" << server_paths["fileindex"];
    SimpleLogger().Write(logDEBUG) << "Names file:\t" << server_paths["namesdata"];
    SimpleLogger().Write(logDEBUG) << "Timestamp file:\t" << server_paths["timestamp"];
}

// generate boost::program_options object for the routing part
inline unsigned GenerateServerProgramOptions(const int argc,
                                             const char *argv[],
                                             ServerPaths &paths,
                                             std::string &ip_address,
                                             int &ip_port,
                                             int &requested_num_threads,
                                             bool &use_shared_memory,
                                             bool &trial)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c",
        boost::program_options::value<boost::filesystem::path>(&paths["config"])
            ->default_value("server.ini"),
        "Path to a configuration file")(
        "trial",
        boost::program_options::value<bool>(&trial)->implicit_value(true),
        "Quit after initialization");

    // declare a group of options that will be allowed both on command line
    // as well as in a config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()(
        "hsgrdata",
        boost::program_options::value<boost::filesystem::path>(&paths["hsgrdata"]),
        ".hsgr file")("nodesdata",
                      boost::program_options::value<boost::filesystem::path>(&paths["nodesdata"]),
                      ".nodes file")(
        "edgesdata",
        boost::program_options::value<boost::filesystem::path>(&paths["edgesdata"]),
        ".edges file")("geometry",
                       boost::program_options::value<boost::filesystem::path>(&paths["geometries"]),
                       ".geometry file")(
        "ramindex",
        boost::program_options::value<boost::filesystem::path>(&paths["ramindex"]),
        ".ramIndex file")(
        "fileindex",
        boost::program_options::value<boost::filesystem::path>(&paths["fileindex"]),
        "File index file")(
        "namesdata",
        boost::program_options::value<boost::filesystem::path>(&paths["namesdata"]),
        ".names file")("timestamp",
                       boost::program_options::value<boost::filesystem::path>(&paths["timestamp"]),
                       ".timestamp file")(
        "ip,i",
        boost::program_options::value<std::string>(&ip_address)->default_value("0.0.0.0"),
        "IP address")(
        "port,p", boost::program_options::value<int>(&ip_port)->default_value(5000), "TCP/IP port")(
        "threads,t",
        boost::program_options::value<int>(&requested_num_threads)->default_value(8),
        "Number of threads to use")(
        "sharedmemory,s",
        boost::program_options::value<bool>(&use_shared_memory)->implicit_value(true),
        "Load data from shared memory");

    // hidden options, will be allowed both on command line and in config
    // file, but will not be shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "base,b",
        boost::program_options::value<boost::filesystem::path>(&paths["base"]),
        "base path to .osrm file");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("base", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        boost::filesystem::basename(argv[0]) + " <base.osrm> [<options>]");
    visible_options.add(generic_options).add(config_options);

    // parse command line options
    boost::program_options::variables_map option_variables;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                      .options(cmdline_options)
                                      .positional(positional_options)
                                      .run(),
                                  option_variables);

    if (option_variables.count("version"))
    {
        SimpleLogger().Write() << g_GIT_DESCRIPTION;
        return INIT_OK_DO_NOT_START_ENGINE;
    }

    if (option_variables.count("help"))
    {
        SimpleLogger().Write() << visible_options;
        return INIT_OK_DO_NOT_START_ENGINE;
    }

    boost::program_options::notify(option_variables);

    // parse config file
    ServerPaths::iterator path_iterator = paths.find("config");
    if (path_iterator != paths.end() && boost::filesystem::is_regular_file(path_iterator->second) &&
        !option_variables.count("base"))
    {
        SimpleLogger().Write() << "Reading options from: " << path_iterator->second.string();
        std::string ini_file_contents = ReadIniFileAndLowerContents(path_iterator->second);
        std::stringstream config_stream(ini_file_contents);
        boost::program_options::store(parse_config_file(config_stream, config_file_options),
                                      option_variables);
        boost::program_options::notify(option_variables);
        return INIT_OK_START_ENGINE;
    }

    if (1 > requested_num_threads)
    {
        throw OSRMException("Number of threads must be a positive number");
    }

    if (!use_shared_memory && option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    if (use_shared_memory && !option_variables.count("base"))
    {
        return INIT_OK_START_ENGINE;
    }
    SimpleLogger().Write() << visible_options;
    return INIT_OK_DO_NOT_START_ENGINE;
}

#endif /* PROGRAM_OPTIONS_H */
