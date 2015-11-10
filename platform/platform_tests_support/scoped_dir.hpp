#pragma once

#include "platform/platform.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"

namespace platform
{
namespace tests_support
{
// Scoped test directory in a writable dir.
class ScopedDir
{
public:
  /// Creates test dir in a writable directory.
  /// @param path Path for a testing directory, should be relative to writable-dir.
  ScopedDir(string const & relativePath);

  ScopedDir(ScopedDir const & parent, string const & name);

  ~ScopedDir();

  inline void Reset() { m_reset = true; }

  inline string const & GetFullPath() const { return m_fullPath; }

  inline string const & GetRelativePath() const { return m_relativePath; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  string const m_relativePath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedDir);
};

string DebugPrint(ScopedDir const & dir);
}  // namespace tests_support
}  // namespace platform
