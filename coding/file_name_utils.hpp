#pragma once

#include <initializer_list>
#include <string>
#include <utility>

namespace base
{
/// Remove extension from file name.
void GetNameWithoutExt(std::string & name);
std::string FilenameWithoutExt(std::string name);
/// @return File extension with the dot or empty std::string if no extension found.
std::string GetFileExtension(std::string const & name);

/// Get file name from full path.
void GetNameFromFullPath(std::string & name);

/// Get file name from full path without extension.
std::string GetNameFromFullPathWithoutExt(std::string const & path);

/// Returns all but last components of the path. After dropping the last
/// component, all trailing slashes are removed, unless the result is a
/// root directory. If the argument is a single component, returns ".".
std::string GetDirectory(std::string const & path);

/// Get folder separator for specific platform
std::string GetNativeSeparator();

/// @deprecated use JoinPath instead.
std::string JoinFoldersToPath(std::string const & folder, std::string const & file);
std::string JoinFoldersToPath(std::initializer_list<std::string> const & folders,
                              std::string const & file);

/// Add the terminating slash to the folder path std::string if it's not already there.
std::string AddSlashIfNeeded(std::string const & path);

inline std::string JoinPath(std::string const & file) { return file; }

/// Create full path from some folder using native folders separator.
template <typename... Args>
std::string JoinPath(std::string const & folder, Args &&... args)
{
  if (folder.empty())
    return JoinPath(std::forward<Args>(args)...);

  return AddSlashIfNeeded(folder) + JoinPath(std::forward<Args>(args)...);
}
}  // namespace base
