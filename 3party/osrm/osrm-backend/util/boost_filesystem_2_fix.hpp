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

#ifndef BOOST_FILE_SYSTEM_FIX_H
#define BOOST_FILE_SYSTEM_FIX_H

#include "osrm_exception.hpp"

// #include <boost/any.hpp>
#include <boost/filesystem.hpp>
// #include <boost/program_options.hpp>

// This is one big workaround for latest boost renaming woes.

#if BOOST_FILESYSTEM_VERSION < 3
#warning Boost Installation with Filesystem3 missing, activating workaround
#include <cstdio>
#endif

namespace boost
{
namespace filesystem
{

// Validator for boost::filesystem::path, that verifies that the file
// exists. The validate() function must be defined in the same namespace
// as the target type, (boost::filesystem::path in this case), otherwise
// it is not called
// inline void validate(
//     boost::any & v,
//     const std::vector<std::string> & values,
//     boost::filesystem::path *,
//     int
// ) {
//     boost::program_options::validators::check_first_occurrence(v);
//     const std::string & input_string =
//         boost::program_options::validators::get_single_string(values);
//     if(boost::filesystem::is_regular_file(input_string)) {
//         v = boost::any(boost::filesystem::path(input_string));
//     } else {
//         throw osrm::exception(input_string + " not found");
//     }
// }

// adapted from:
// http://stackoverflow.com/questions/1746136/how-do-i-normalize-a-pathname-using-boostfilesystem
inline boost::filesystem::path
portable_canonical(const boost::filesystem::path &relative_path,
                   const boost::filesystem::path &current_path = boost::filesystem::current_path())
{
    const boost::filesystem::path absolute_path =
        boost::filesystem::absolute(relative_path, current_path);

    boost::filesystem::path canonical_path;
    for (auto path_iterator = absolute_path.begin(); path_iterator != absolute_path.end();
         ++path_iterator)
    {
        if (".." == path_iterator->string())
        {
            // /a/b/.. is not necessarily /a if b is a symbolic link
            if (boost::filesystem::is_symlink(canonical_path))
            {
                canonical_path /= *path_iterator;
            }
            else if (".." == canonical_path.filename())
            {
                // /a/b/../.. is not /a/b/.. under most circumstances
                // We can end up with ..s in our result because of symbolic links
                canonical_path /= *path_iterator;
            }
            else
            {
                // Otherwise it should be safe to resolve the parent
                canonical_path = canonical_path.parent_path();
            }
        }
        else if ("." == path_iterator->string())
        {
            // Ignore
        }
        else
        {
            // Just cat other path entries
            canonical_path /= *path_iterator;
        }
    }
    BOOST_ASSERT(canonical_path.is_absolute());
    BOOST_ASSERT(boost::filesystem::exists(canonical_path));
    return canonical_path;
}

#if BOOST_FILESYSTEM_VERSION < 3

inline path temp_directory_path()
{
    char *buffer;
    buffer = tmpnam(nullptr);

    return path(buffer);
}

inline path unique_path(const path &) { return temp_directory_path(); }

#endif
}
}

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#endif

inline void AssertPathExists(const boost::filesystem::path &path)
{
    if (!boost::filesystem::is_regular_file(path))
    {
        throw osrm::exception(path.string() + " not found.");
    }
}

#endif /* BOOST_FILE_SYSTEM_FIX_H */
