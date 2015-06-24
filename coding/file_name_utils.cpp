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
    return folder + GetNativeSeparator() + file;
}

string JoinFoldersToPath(const string & folder1, const string & folder2, const string & file)
{
    string nativeSeparator = GetNativeSeparator();
    return folder1 + nativeSeparator + folder2 + nativeSeparator + file;
}

string JoinFoldersToPath(const string & folder1, const string & folder2, const string & folder3, const string & file)
{
    string nativeSeparator = GetNativeSeparator();
    return folder1 + nativeSeparator + folder2 + nativeSeparator + folder3 + nativeSeparator + file;
}

string JoinFoldersToPath(const vector<string> & folders, const string & file)
{
    if (folders.empty())
        return file;

    string nativeSeparator = GetNativeSeparator();
    string result;
    for (size_t i = 0; i < folders.size(); ++i)
        result = result + folders[i] + nativeSeparator;

    result += file;
    return result;
}
}  // namespace my
