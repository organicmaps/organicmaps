#pragma once

#include "../std/string.hpp"

/// extract folder name from full path. ended with path delimiter
string const extract_folder(string const & fileName);
/// extract file name from full path.
string const extract_name(string const & fileName);
