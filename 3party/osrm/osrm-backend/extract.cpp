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

#include "extractor/extractor.hpp"
#include "extractor/extractor_options.hpp"
#include "util/simple_logger.hpp"

#include <boost/filesystem.hpp>

#include <exception>

int main(int argc, char *argv[])
{
    try
    {
        LogPolicy::GetInstance().Unmute();
        ExtractorConfig extractor_config;

        const return_code result = ExtractorOptions::ParseArguments(argc, argv, extractor_config);

        if (return_code::fail == result)
        {
            return 1;
        }

        if (return_code::exit == result)
        {
            return 0;
        }

        ExtractorOptions::GenerateOutputFilesNames(extractor_config);

        if (1 > extractor_config.requested_num_threads)
        {
            SimpleLogger().Write(logWARNING) << "Number of threads must be 1 or larger";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(extractor_config.input_path))
        {
            SimpleLogger().Write(logWARNING)
                << "Input file " << extractor_config.input_path.string() << " not found!";
            return 1;
        }

        if (!boost::filesystem::is_regular_file(extractor_config.profile_path))
        {
            SimpleLogger().Write(logWARNING) << "Profile " << extractor_config.profile_path.string()
                                             << " not found!";
            return 1;
        }
        return extractor().run(extractor_config);
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
        return 1;
    }
}
