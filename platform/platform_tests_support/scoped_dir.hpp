#pragma once

#include "platform/platform.hpp"

#include "base/macros.hpp"

#include <string>

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
  ScopedDir(std::string const & relativePath);

  ScopedDir(ScopedDir const & parent, std::string const & name);

  ~ScopedDir();

  void Reset() { m_reset = true; }

  std::string const & GetFullPath() const { return m_fullPath; }

  std::string const & GetRelativePath() const { return m_relativePath; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  std::string const m_fullPath;
  std::string const m_relativePath;
  bool m_reset;

  DISALLOW_COPY_AND_MOVE(ScopedDir);
};

std::string DebugPrint(ScopedDir const & dir);
}  // namespace tests_support
}  // namespace platform
