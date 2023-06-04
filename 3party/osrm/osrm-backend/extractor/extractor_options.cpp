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

#include "extractor_options.hpp"

#include "../util/git_sha.hpp"
#include "../util/ini_file.hpp"
#include "../util/simple_logger.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <tbb/task_scheduler_init.h>

return_code
ExtractorOptions::ParseArguments(int argc, char *argv[], ExtractorConfig &extractor_config)
{
    // declare a group of options that will be allowed only on command line
    boost::program_options::options_description generic_options("Options");
    generic_options.add_options()("version,v", "Show version")("help,h", "Show this help message")(
        "config,c", boost::program_options::value<boost::filesystem::path>(
                        &extractor_config.config_file_path)->default_value("extractor.ini"),
        "Path to a configuration file.");

    // declare a group of options that will be allowed both on command line and in config file
    boost::program_options::options_description config_options("Configuration");
    config_options.add_options()("profile,p",
                                 boost::program_options::value<boost::filesystem::path>(
                                     &extractor_config.profile_path)->default_value("profile.lua"),
                                 "Path to LUA routing profile")(
        "threads,t",
        boost::program_options::value<unsigned int>(&extractor_config.requested_num_threads)
            ->default_value(tbb::task_scheduler_init::default_num_threads()),
        "Number of threads to use");

    // hidden options, will be allowed both on command line and in config file, but will not be
    // shown to the user
    std::string string_input_path;
    boost::program_options::options_description hidden_options("Hidden options");
    hidden_options.add_options()("input,i", boost::program_options::value<std::string>(
                                                &string_input_path),
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
    try
    {
        boost::program_options::variables_map option_variables;
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                          .options(cmdline_options)
                                          .positional(positional_options)
                                          .run(),
                                      option_variables);
        if (option_variables.count("version"))
        {
            SimpleLogger().Write() << g_GIT_DESCRIPTION;
            return return_code::exit;
        }

        if (option_variables.count("help"))
        {
            SimpleLogger().Write() << visible_options;
            return return_code::exit;
        }

        boost::program_options::notify(option_variables);

        // parse config file
        if (boost::filesystem::is_regular_file(extractor_config.config_file_path))
        {
            SimpleLogger().Write()
                << "Reading options from: " << extractor_config.config_file_path.string();
            std::string ini_file_contents =
                read_file_lower_content(extractor_config.config_file_path);
            std::stringstream config_stream(ini_file_contents);
            boost::program_options::store(parse_config_file(config_stream, config_file_options),
                                          option_variables);
            boost::program_options::notify(option_variables);
        }

        if (!option_variables.count("input"))
        {
            SimpleLogger().Write() << visible_options;
            return return_code::exit;
        }
    }
    catch (std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << e.what();
        return return_code::fail;
    }

   extractor_config.input_path = string_input_path;

    return return_code::ok;
}

void ExtractorOptions::GenerateOutputFilesNames(ExtractorConfig &extractor_config)
{
    boost::filesystem::path &input_path = extractor_config.input_path;
    extractor_config.output_file_name = input_path.string();
    extractor_config.restriction_file_name = input_path.string();
    extractor_config.timestamp_file_name = input_path.string();
    std::string::size_type pos = extractor_config.output_file_name.find(".osm.bz2");
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".osm.pbf");
        if (pos == std::string::npos)
        {
            pos = extractor_config.output_file_name.find(".osm.xml");
        }
    }
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".pbf");
    }
    if (pos == std::string::npos)
    {
        pos = extractor_config.output_file_name.find(".osm");
        if (pos == std::string::npos)
        {
            extractor_config.output_file_name.append(".osrm");
            extractor_config.restriction_file_name.append(".osrm.restrictions");
            extractor_config.timestamp_file_name.append(".osrm.timestamp");
        }
        else
        {
            extractor_config.output_file_name.replace(pos, 5, ".osrm");
            extractor_config.restriction_file_name.replace(pos, 5, ".osrm.restrictions");
            extractor_config.timestamp_file_name.replace(pos, 5, ".osrm.timestamp");
        }
    }
    else
    {
        extractor_config.output_file_name.replace(pos, 8, ".osrm");
        extractor_config.restriction_file_name.replace(pos, 8, ".osrm.restrictions");
        extractor_config.timestamp_file_name.replace(pos, 8, ".osrm.timestamp");
    }
}
