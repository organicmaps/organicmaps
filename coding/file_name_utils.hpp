#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace my
{
  /// Remove extension from file name.
  void GetNameWithoutExt(string & name);
  /// @return File extension with the dot or empty string if no extension found.
  string GetFileExtension(string const & name);

  /// Get file name from full path.
  void GetNameFromFullPath(string & name);

  /// Returns all but last components of the path. After dropping the last
  /// component, all trailing slashes are removed, unless the result is a
  /// root directory. If the argument is a single component, returns ".".
  string GetDirectory(string const & path);

  /// Get folder separator for specific platform
  string GetNativeSeparator();

  /// Create full path from some folder using native folders separator
  string JoinFoldersToPath(const string & folder, const string & file);
  string JoinFoldersToPath(const string & folder1, const string & folder2, const string & file);
  string JoinFoldersToPath(const string & folder1, const string & folder2, const string & folder3, const string & file);
  string JoinFoldersToPath(const vector<string> & folders, const string & file);

  /// Add terminating slash, if it's not exist to the folder path string.
  string AddSlashIfNeeded(string const & path);
}
