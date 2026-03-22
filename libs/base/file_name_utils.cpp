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

std::string GetDirectory(std::string const & name)
{
  auto const sep = GetNativeSeparator();
  size_t const sepSize = sizeof(sep);

  std::string::size_type i = name.rfind(sep);
  if (i == std::string::npos)
    return ".";
  while (i > sepSize && (name[i - sepSize] == sep))
    i -= sepSize;
  return name.substr(0, i ? i : sepSize);
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
  auto const sep = GetNativeSeparator();
  std::string::size_type const pos = path.rfind(sep);
  if (pos != std::string::npos && pos + sizeof(sep) == path.size())
    return path;
  return path + sep;
}
}  // namespace base
