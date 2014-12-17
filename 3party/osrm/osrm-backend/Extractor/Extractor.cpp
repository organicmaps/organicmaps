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

#include "Extractor.h"

#include "ExtractorCallbacks.h"
#include "ExtractionContainers.h"
#include "PBFParser.h"
#include "ScriptingEnvironment.h"
#include "XMLParser.h"

#include "../Util/GitDescription.h"
#include "../Util/IniFileUtil.h"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../Util/TimingUtil.h"
#include "../typedefs.h"

#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

#include <cstdlib>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>

Extractor::Extractor() : requested_num_threads(0), file_has_pbf_format(false)
{
}

Extractor::~Extractor() {}

bool Extractor::ParseArguments(int argc, char *argv[])
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c",
        boost::program_options::value<boost::filesystem::path>(&config_file_path)
            ->default_value("extractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()("profile,p",
                                 boost::program_options::value<boost::filesystem::path>(
                                     &profile_path)->default_value("profile.lua"),
                                 "Path to LUA routing profile")(
        "threads,t",
        boost::program_options::value<unsigned int>(&requested_num_threads)
            ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use");

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()(
        "input,i",
        boost::program_options::value<boost::filesystem::path>(&input_path),
        "Input file in .osm, .osm.bz2 or .osm.pbf format");

    // positional option
    boost::program_options::positional_options_description positional_options;
    positional_options.add("input", 1);

    // combine above options for parsing
    boost::program_options::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_options).add(hidden_options);

    boost::program_options::options_description config_file_options;
    config_file_options.add(config_options).add(hidden_options);

    boost::program_options::options_description visible_options(
        boost::filesystem::basename(argv[0]) + " <input.osm/.osm.bz2/.osm.pbf> [options]");
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
        return false;
    }

    if (option_variables.count("help"))
    {
        SimpleLogger().Write() << visible_options;
        return false;
    }

    boost::program_options::notify(option_variables);

    // parse config file
    if (boost::filesystem::is_regular_file(config_file_path))
    {
        SimpleLogger().Write() << "Reading options from: " << config_file_path.string();
        std::string ini_file_contents = ReadIniFileAndLowerContents(config_file_path);
        std::stringstream config_stream(ini_file_contents);
        boost::program_options::store(parse_config_file(config_stream, config_file_options),
                                      option_variables);
        boost::program_options::notify(option_variables);
    }

    if (!option_variables.count("input"))
    {
        SimpleLogger().Write() << visible_options;
        return false;
    }

    return true;
}

void Extractor::GenerateOutputFilesNames()
{
    output_file_name = input_path.string();
    restriction_file_name = input_path.string();
    std::string::size_type pos = output_file_name.find(".osm.bz2");
    if (pos == std::string::npos)
    {
        pos = output_file_name.find(".osm.pbf");
        if (pos != std::string::npos)
        {
            file_has_pbf_format = true;
        }
        else
        {
            pos = output_file_name.find(".osm.xml");
        }
    }
    if (pos == std::string::npos)
    {
        pos = output_file_name.find(".pbf");
        if (pos != std::string::npos)
        {
            file_has_pbf_format = true;
        }
    }
    if (pos == std::string::npos)
    {
        pos = output_file_name.find(".osm");
        if (pos == std::string::npos)
        {
            output_file_name.append(".osrm");
            restriction_file_name.append(".osrm.restrictions");
        }
        else
        {
            output_file_name.replace(pos, 5, ".osrm");
            restriction_file_name.replace(pos, 5, ".osrm.restrictions");
        }
    }
    else
    {
        output_file_name.replace(pos, 8, ".osrm");
        restriction_file_name.replace(pos, 8, ".osrm.restrictions");
    }
}

int Extractor::Run(int argc, char *argv[])
{
    try
    {
        LogPolicy::GetInstance().Unmute();

        TIMER_START(extracting);

        if (!ParseArguments(argc, argv))
            return 0;

        if (1 > requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(input_path))
        {
            SimpleLogger().Write(logWARNING) << "Input file " << input_path.string()
                                             << " not found!";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(profile_path))
        {
            SimpleLogger().Write(logWARNING) << "Profile " << profile_path.string()
                                             << " not found!";
            return 1;
        }

        const unsigned recommended_num_threads = tbb::task_scheduler_init::default_num_threads();

        SimpleLogger().Write() << "Input file: " << input_path.filename().string();
        SimpleLogger().Write() << "Profile: " << profile_path.filename().string();
        SimpleLogger().Write() << "Threads: " << requested_num_threads;
        if (recommended_num_threads != requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "The recommended number of threads is "
                                             << recommended_num_threads
                                             << "! This setting may have performance side-effects.";
        }

        tbb::task_scheduler_init init(requested_num_threads);

        /*** Setup Scripting Environment ***/
        ScriptingEnvironment scripting_environment(profile_path.string().c_str());

        GenerateOutputFilesNames();

        std::unordered_map<std::string, NodeID> string_map;
        ExtractionContainers extraction_containers;

        string_map[""] = 0;
        auto extractor_callbacks = new ExtractorCallbacks(extraction_containers, string_map);
        BaseParser *parser;
        if (file_has_pbf_format)
        {
            parser = new PBFParser(input_path.string().c_str(),
                                   extractor_callbacks,
                                   scripting_environment,
                                   requested_num_threads);
        }
        else
        {
            parser = new XMLParser(input_path.string().c_str(),
                                   extractor_callbacks,
                                   scripting_environment);
        }

        if (!parser->ReadHeader())
        {
            throw OSRMException("Parser not initialized!");
        }

        SimpleLogger().Write() << "Parsing in progress..";
        TIMER_START(parsing);

        parser->Parse();
        delete parser;
        delete extractor_callbacks;

        TIMER_STOP(parsing);
        SimpleLogger().Write() << "Parsing finished after " << TIMER_SEC(parsing) << " seconds";

        if (extraction_containers.all_edges_list.empty())
        {
            SimpleLogger().Write(logWARNING) << "The input data is empty, exiting.";
            return 1;
        }

        extraction_containers.PrepareData(output_file_name, restriction_file_name);

        TIMER_STOP(extracting);
        SimpleLogger().Write() << "extraction finished after " << TIMER_SEC(extracting) << "s";
        SimpleLogger().Write() << "To prepare the data for routing, run: "
                               << "./osrm-prepare " << output_file_name << std::endl;
    }
    catch (boost::program_options::too_many_positional_options_error &)
    {
        SimpleLogger().Write(logWARNING) << "Only one input file can be specified";
        return 1;
    }
    catch (std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << e.what();
        return 1;
    }
    return 0;
}
