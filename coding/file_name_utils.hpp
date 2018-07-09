#pragma once

#include "std/initializer_list.hpp"
#include "std/string.hpp"

#include <utility>

namespace my
{
  /// Remove extension from file name.
  void GetNameWithoutExt(string & name);
  string FilenameWithoutExt(string name);
  /// @return File extension with the dot or empty string if no extension found.
  string GetFileExtension(string const & name);

  /// Get file name from full path.
  void GetNameFromFullPath(string & name);

  /// Get file name from full path without extension.
  string GetNameFromFullPathWithoutExt(string const & path);

  /// Returns all but last components of the path. After dropping the last
  /// component, all trailing slashes are removed, unless the result is a
  /// root directory. If the argument is a single component, returns ".".
  string GetDirectory(string const & path);

  /// Get folder separator for specific platform
  string GetNativeSeparator();

  /// @deprecated use JoinPath instead.
  string JoinFoldersToPath(const string & folder, const string & file);
  string JoinFoldersToPath(initializer_list<string> const & folders, const string & file);

  /// Add the terminating slash to the folder path string if it's not already there.
  string AddSlashIfNeeded(string const & path);

  inline std::string JoinPath(std::string const & file) { return file; }

  /// Create full path from some folder using native folders separator.
  template<typename... Args>
  std::string JoinPath(std::string const & folder, Args&&... args)
  {
    return AddSlashIfNeeded(folder) + JoinPath(std::forward<Args>(args)...);
  }
}
