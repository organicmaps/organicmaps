#include "base/file_name_utils.hpp"

#include <string_view>

namespace base
{
namespace
{
#ifdef OMIM_OS_WINDOWS
// Windows accepts forward slashes too, and CMake or URL derived paths use them.
std::string_view constexpr kSeparators = "/\\";
#else
std::string_view constexpr kSeparators = "/";
#endif
}  // namespace

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
  auto const last = name.find_last_of(kSeparators);
  if (last == std::string::npos)
    return ".";
  // Drop the run of separators before the file name, but keep a leading one.
  auto const end = name.find_last_not_of(kSeparators, last);
  return name.substr(0, end == std::string::npos ? 1 : end + 1);
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
  if (!path.empty() && kSeparators.find(path.back()) != std::string_view::npos)
    return path;
  return path + GetNativeSeparator();
}
}  // namespace base
