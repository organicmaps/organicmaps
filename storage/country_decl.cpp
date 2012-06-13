#include "country_decl.hpp"


string storage::CountryInfo::FileName2FullName(string fName)
{
  size_t const i = fName.find('_');
  if (i != string::npos)
  {
    // replace '_' with ", "
    fName[i] = ',';
    fName.insert(i+1, " ");
  }
  return fName;
}

void storage::CountryInfo::FullName2GroupAndMap(string const & fName, string & group, string & map)
{
  size_t pos = fName.find(",");
  if (pos == string::npos)
  {
    map = fName;
  }
  else
  {
    map = fName.substr(pos + 2);
    group = fName.substr(0, pos);
  }
}
