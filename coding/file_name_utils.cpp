#include "file_name_utils.hpp"

#include "std/target_os.hpp"


namespace my
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

string JoinFoldersToPath(const string & folder, const string & file)
{
  return my::AddSlashIfNeeded(folder) + file;
}

string JoinFoldersToPath(initializer_list<string> const & folders, const string & file)
{
  string result;
  for (string const & s : folders)
    result += AddSlashIfNeeded(s);

  result += file;
  return result;
}

string AddSlashIfNeeded(string const & path)
{
  string const sep = GetNativeSeparator();
  string::size_type const pos = path.rfind(sep);
  if ((pos != string::npos) && (pos + sep.size() == path.size()))
    return path;
  return path + sep;
}

}  // namespace my
