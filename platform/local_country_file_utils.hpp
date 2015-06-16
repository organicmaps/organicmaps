#pragma once

#include "platform/local_country_file.hpp"

#include "std/shared_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace platform
{
// Removes partially downloaded maps, empty directories and old
// (format v1) maps.  Also, removes old (splitted) Japan and Brazil
// maps.
void CleanupMapsDirectory();

// Finds all local map files in a directory. Version of these files is
// passed as an argument.
void FindAllLocalMapsInDirectory(string const & directory, int64_t version,
                                 vector<LocalCountryFile> & localFiles);

// Finds all local map files in resources and writable
// directory. Also, for Android, checks /Android/obb directory.
// Directories should have the following structure:
//
// dir/*.mwm            -- map files, base name should correspond to countries.txt,
//                      -- version is assumed to be zero (unknown).
// dir/*.mwm.routing    -- routing files for corresponding map files,
//                      -- version is assumed to be zero (unknown).
// dir/[0-9]+/*.mwm     -- map files, base name should correspond to countries.txt,
//                      -- version is assumed to be the name of a directory.
// dir/[0-9]{1,18}/*.mwm.routing  -- routing file for corresponding map files,
//                                -- version is assumed to be the name of a directory.
//
// We can't derive version of a map file from its header because
// currently header stores format version + individual mwm's file
// generation timestamp, not timestamp mentioned in countries.txt.
void FindAllLocalMaps(vector<LocalCountryFile> & localFiles);

// Tries to parse a version from a string of size not longer than 18
// symbols and representing an unsigned decimal number. Leading zeroes
// are allowed.
bool ParseVersion(string const & s, int64_t & version);

// When version is zero, uses writable directory, otherwise, creates
// directory with name equal to decimal representation of version.
shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(CountryFile const & countryFile,
                                                         int64_t version);
}  // namespace platform
