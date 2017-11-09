#pragma once

#include "platform/country_defines.hpp"
#include "platform/platform.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"

namespace platform
{
class CountryFile;

namespace tests_support
{
class ScopedDir;

class ScopedFile
{
public:
  explicit ScopedFile(string const & relativePath);

  ScopedFile(string const & relativePath, string const & contents);

  ScopedFile(ScopedDir const & dir, CountryFile const & countryFile, MapOptions file,
             string const & contents);

  ~ScopedFile();

  inline string const & GetFullPath() const { return m_fullPath; }

  inline void Reset() { m_reset = true; }

  inline bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  string const m_fullPath;
  bool m_reset = false;

  DISALLOW_COPY_AND_MOVE(ScopedFile);
};

string DebugPrint(ScopedFile const & file);
}  // namespace tests_support
}  // namespace platform
