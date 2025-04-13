#pragma once

#include "platform/country_defines.hpp"
#include "platform/platform.hpp"

#include "base/macros.hpp"

#include <string>

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
  ScopedFile(std::string const & relativePath, Mode mode);

  // Creates a scoped file in Mode::Create and writes |contents| to it.
  ScopedFile(std::string const & relativePath, std::string const & contents);

  // Creates a scoped file in Mode::Create using the path inferred from |countryFile|
  // and |mapOptions|.
  ScopedFile(ScopedDir const & dir, CountryFile const & countryFile, MapFileType type);

  ~ScopedFile();

  std::string const & GetFullPath() const { return m_fullPath; }

  void Reset() { m_reset = true; }

  bool Exists() const { return GetPlatform().IsFileExistsByFullPath(GetFullPath()); }

private:
  ScopedFile(std::string const & relativePath, std::string const & contents, Mode mode);

  std::string const m_fullPath;
  bool m_reset = false;

  DISALLOW_COPY_AND_MOVE(ScopedFile);
};

std::string DebugPrint(ScopedFile const & file);
}  // namespace tests_support
}  // namespace platform
