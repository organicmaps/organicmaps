#include "file_name_utils.hpp"


namespace my
{

void GetNameWithoutExt(string & name)
{
  string::size_type const i = name.find_last_of(".");
  if (i != string::npos)
    name.erase(i);
}

void GetNameFromFullPath(string & name)
{
  string::size_type const i = name.find_last_of("/\\");
  if (i != string::npos)
    name = name.substr(i+1);
}

}
