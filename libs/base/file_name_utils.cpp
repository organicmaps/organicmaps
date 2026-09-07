#include "base/file_name_utils.hpp"

namespace base
{
void GetNameWithoutExt(std::string & name)
{
  std::string::size_type const i = name.rfind('.');
  if (i != std::string::npos)
    name.erase(i);
}

std::string FilenameWithoutExt(std::string name)
{
  GetNameWithoutExt(name);
  return name;
}

std::string GetFileExtension(std::string const & name)
{
  size_t const pos = name.find_last_of("./\\");
  return ((pos != std::string::npos && name[pos] == '.') ? name.substr(pos) : std::string());
}

void GetNameFromFullPath(std::string & name)
{
  std::string::size_type const i = name.find_last_of("/\\");
  if (i != std::string::npos)
    name = name.substr(i + 1);
}

std::string FileNameFromFullPath(std::string path)
{
  GetNameFromFullPath(path);
  return path;
}

std::string GetNameFromFullPathWithoutExt(std::string path)
{
  GetNameFromFullPath(path);
  GetNameWithoutExt(path);
  return path;
}

// Windows accepts forward slashes too, and CMake or URL derived paths use them.
bool IsSeparator(char c)
{
#ifdef OMIM_OS_WINDOWS
  return c == '/' || c == '\\';
#else
  return c == '/';
#endif
}

std::string GetDirectory(std::string const & name)
{
  // Index right after the last separator.
  std::string::size_type i = name.size();
  while (i > 0 && !IsSeparator(name[i - 1]))
    --i;
  if (i == 0)
    return ".";
  // Skip the run of separators before the file name, but keep a leading one.
  --i;
  while (i > 0 && IsSeparator(name[i - 1]))
    --i;
  return name.substr(0, i ? i : 1);
}

std::string::value_type GetNativeSeparator()
{
#ifdef OMIM_OS_WINDOWS
  return '\\';
#else
  return '/';
#endif
}

std::string AddSlashIfNeeded(std::string const & path)
{
  if (!path.empty() && IsSeparator(path.back()))
    return path;
  return path + GetNativeSeparator();
}
}  // namespace base
