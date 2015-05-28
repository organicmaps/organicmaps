#pragma once

#include "platform/local_country_file.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"

namespace local_country_file_utils
{
void FindAllLocalMapsInDirectory(string const & directory, int64_t version,
                                 vector<LocalCountryFile> & localFiles);
void FindAllLocalMaps(vector<LocalCountryFile> & localFiles);

bool ParseVersion(string const & s, int64_t & version);
}  // namespace local_country_file_utils
