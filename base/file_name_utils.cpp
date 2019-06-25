#include "base/file_name_utils.hpp"

#include "std/target_os.hpp"

using namespace std;

namespace base
{
void GetNameWithoutExt(string & name)
{
  string::size_type const i = name.rfind('.');
  if (i != string::npos)
    name.erase(i);
}

string FilenameWithoutExt(string name)
{
  GetNameWithoutExt(name);
  return name;
}

string GetFileExtension(string const & name)
{
  size_t const pos = name.find_last_of("./\\");
  return ((pos != string::npos && name[pos] == '.') ? name.substr(pos) : string());
}

void GetNameFromFullPath(string & name)
{
  string::size_type const i = name.find_last_of("/\\");
  if (i != string::npos)
    name = name.substr(i+1);
}

std::string GetNameFromFullPath(std::string const & path)
{
  std::string name = path;
  GetNameFromFullPath(name);
  return name;
}

string GetNameFromFullPathWithoutExt(string const & path)
{
  string name = path;
  GetNameFromFullPath(name);
  GetNameWithoutExt(name);
  return name;
}

string GetDirectory(string const & name)
{
  string const sep = GetNativeSeparator();
  size_t const sepSize = sep.size();

  string::size_type i = name.rfind(sep);
  if (i == string::npos)
    return ".";
  while (i > sepSize && (name.substr(i - sepSize, sepSize) == sep))
    i -= sepSize;
  return i == 0 ? sep : name.substr(0, i);
}

string GetNativeSeparator()
{
#ifdef OMIM_OS_WINDOWS
    return "\\";
#else
    return "/";
#endif
}

string AddSlashIfNeeded(string const & path)
{
  string const sep = GetNativeSeparator();
  string::size_type const pos = path.rfind(sep);
  if ((pos != string::npos) && (pos + sep.size() == path.size()))
    return path;
  return path + sep;
}
}  // namespace base
