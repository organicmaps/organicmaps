#include "storage/country_decl.hpp"

#include <cstddef>

namespace storage
{
// static
void CountryInfo::FileName2FullName(std::string & fName)
{
  size_t const i = fName.find('_');
  if (i != std::string::npos)
  {
    // replace '_' with ", "
    fName[i] = ',';
    fName.insert(i + 1, " ");
  }
}

// static
void CountryInfo::FullName2GroupAndMap(std::string const & fName, std::string & group, std::string & map)
{
  size_t pos = fName.find(',');
  if (pos == std::string::npos)
  {
    map = fName;
    group.clear();
  }
  else
  {
    map = fName.substr(pos + 2);
    group = fName.substr(0, pos);
  }
}
}  // namespace storage
