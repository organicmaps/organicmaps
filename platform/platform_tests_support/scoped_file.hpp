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
  enum class Mode : uint32_t
  {
    // Create or overwrite the file and remove it at scope exit.
    Create,

    // Remove the file at scope exit. The caller must
    // ensure that the file has been created by that time.
    DoNotCreate
  };

  // Creates a scoped file in the specified mode.
  ScopedFile(string const & relativePath, Mode mode);

  // Creates a scoped file in Mode::Create and writes |contents| to it.
  ScopedFile(string const & relativePath, string const & contents);

  // Creates a scoped file in Mode::Create using the path inferred from |countryFile|
  // and |mapOptions|.
  ScopedFile(ScopedDir const & dir, CountryFile const & countryFile, MapOptions mapOptions);

  ~ScopedFile();

  inline string const & GetFullPath() const { return m_fullPath; }

  inline void Reset() { m_reset = true; }

  inline bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  ScopedFile(string const & relativePath, string const & contents, Mode mode);

  string const m_fullPath;
  bool m_reset = false;

  DISALLOW_COPY_AND_MOVE(ScopedFile);
};

string DebugPrint(ScopedFile const & file);
}  // namespace tests_support
}  // namespace platform
