#pragma once

#include "platform/country_file.hpp"
#include "platform/country_defines.hpp"

#include "base/stl_helpers.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace platform
{
// This class represents a path to disk files corresponding to some
// country region.
//
// This class also wraps World.mwm and WorldCoasts.mwm
// files from resource bundle, when they can't be found in a data
// directory. In this exceptional case, directory will be empty and
// SyncWithDisk()/DeleteFromDisk()/GetPath()/GetSize() will return
// incorrect results.
//
// In any case, when you're going to read a file LocalCountryFile points to,
// use platform::GetCountryReader().
class LocalCountryFile
{
public:
  LocalCountryFile();

  // Creates an instance holding a path to countryFile's in a
  // directory. Note that no disk operations are not performed until
  // SyncWithDisk() is called.
  // The directory must contain a full path to the country file.
  LocalCountryFile(std::string const & directory, CountryFile const & countryFile, int64_t version);

  // Syncs internal state like availability of files, their sizes etc. with disk.
  // Generality speaking it's not always true. To know it for sure it's necessary to read a mwm in
  // this method but it's not implemented by performance reasons. This check is done on
  // building routes stage.
  void SyncWithDisk();

  // Removes specified file from disk if it is known for LocalCountryFile, i.e.
  // it was found by a previous SyncWithDisk() call.
  void DeleteFromDisk(MapFileType type) const;

  // Returns path to a file.
  // Return value may be empty until SyncWithDisk() is called.
  std::string GetPath(MapFileType type) const;
  std::string GetFileName(MapFileType type) const;

  // Returns size of a file.
  // Return value may be zero until SyncWithDisk() is called.
  uint64_t GetSize(MapFileType type) const;

  // Returns true when some files are found during SyncWithDisk.
  // Return value is false until SyncWithDisk() is called.
  bool HasFiles() const;

  // Checks whether files specified in filesMask are on disk.
  // Return value will be false until SyncWithDisk() is called.
  bool OnDisk(MapFileType type) const;

  bool IsInBundle() const { return m_directory.empty(); }
  std::string const & GetDirectory() const { return m_directory; }
  std::string const & GetCountryName() const { return m_countryFile.GetName(); }
  int64_t GetVersion() const { return m_version; }
  CountryFile const & GetCountryFile() const { return m_countryFile; }

  bool operator<(LocalCountryFile const & rhs) const;
  bool operator==(LocalCountryFile const & rhs) const;
  bool operator!=(LocalCountryFile const & rhs) const { return !(*this == rhs); }

  bool ValidateIntegrity() const;

  // Creates LocalCountryFile for test purposes, for a country region
  // with countryFileName (without any extensions). Automatically performs sync with disk.
  static LocalCountryFile MakeForTesting(std::string const & countryFileName, int64_t version = 0);

  // Used in generator only to simplify getting instance from path.
  static LocalCountryFile MakeTemporary(std::string const & fullPath);

private:
  friend std::string DebugPrint(LocalCountryFile const &);
  friend void FindAllLocalMapsAndCleanup(int64_t latestVersion, std::string const & dataDir,
                                         std::vector<LocalCountryFile> & localFiles);

  /// Can be bundled (empty directory) or path to the file.
  std::string m_directory;
  CountryFile m_countryFile;
  int64_t m_version;

  using File = std::optional<uint64_t>;
  std::array<File, base::Underlying(MapFileType::Count)> m_files = {};
};

std::string DebugPrint(LocalCountryFile const & file);
}  // namespace platform
