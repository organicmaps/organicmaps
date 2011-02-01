#include "../base/SRC_FIRST.hpp"
#include "path_utils.hpp"
#include "string_utils.hpp"

string const extract_folder(string const & fileName)
{
}

string const extract_name(string const & fileName)
{
  return fileName.substr(fileName.find_last_of("\\/") + 1, fileName.size());
}
