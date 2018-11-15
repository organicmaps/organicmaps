#pragma once

#include "platform/country_file.hpp"
#include "platform/country_defines.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

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
// TODO (@gorshenin): fix this hack somehow
// (https://trello.com/c/qcveFw3M/27-world-worldcoasts-mwm-localcountryfile)
//
// In any case, when you're going to read a file LocalCountryFile points to,
// use GetCountryReader().
class LocalCountryFile
{
public:
  // Creates empty instance.
  LocalCountryFile();

  // Creates an instance holding a path to countryFile's in a
  // directory. Note that no disk operations are not performed until
  // SyncWithDisk() is called.
  // The directory must containt a full path to the country file.
  LocalCountryFile(string const & directory, CountryFile const & countryFile, int64_t version);

  // Syncs internal state like availability of map and routing files,
  // their sizes etc. with disk.
  // In case of one component (single) mwm this method assumed the every mwm has a routing section.
  // Generality speaking it's not always true. To know it for sure it's necessary to read a mwm in
  // this method but it's not implemented by performance reasons. This check is done on
  // building routes stage.
  void SyncWithDisk();

  // Removes specified files from disk if they're known for LocalCountryFile, i.e.
  // were found by previous SyncWithDisk() call.
  void DeleteFromDisk(MapOptions files) const;

  // Returns path to a file. Return value may be empty until
  // SyncWithDisk() is called.
  string GetPath(MapOptions file) const;

  // Returns size of a file. Return value may be zero until
  // SyncWithDisk() is called.
  uint64_t GetSize(MapOptions filesMask) const;

  // Returns a mask of all known country files. Return value may be
  // empty until SyncWithDisk() is called.
  MapOptions GetFiles() const { return m_files; }

  // Checks whether files specified in filesMask are on disk. Return
  // value will be false until SyncWithDisk() is called.
  bool OnDisk(MapOptions filesMask) const
  {
    return (static_cast<unsigned>(m_files) & static_cast<unsigned>(filesMask)) ==
           static_cast<unsigned>(filesMask);
  }

  string const & GetDirectory() const { return m_directory; }
  string const & GetCountryName() const { return m_countryFile.GetName(); }
  int64_t GetVersion() const { return m_version; }
  CountryFile const & GetCountryFile() const { return m_countryFile; }
  CountryFile & GetCountryFile() { return m_countryFile; }

  bool operator<(LocalCountryFile const & rhs) const;
  bool operator==(LocalCountryFile const & rhs) const;
  bool operator!=(LocalCountryFile const & rhs) const { return !(*this == rhs); }

  bool ValidateIntegrity() const;

  // Creates LocalCountryFile for test purposes, for a country region
  // with countryFileName (without any extensions). Automatically
  // performs sync with disk.
  static LocalCountryFile MakeForTesting(string const & countryFileName, int64_t version = 0);

  /// @todo The easiest solution for now. Need to be removed in future.
  /// @param fullPath Full path to the mwm file.
  static LocalCountryFile MakeTemporary(string const & fullPath);

private:
  friend string DebugPrint(LocalCountryFile const &);
  friend void UnitTest_LocalCountryFile_DirectoryLookup();
  friend void FindAllLocalMapsAndCleanup(int64_t latestVersion,
                                         string const & dataDir, vector<LocalCountryFile> & localFiles);

  /// @note! If directory is empty, the file is stored in resources.
  /// In this case, the only valid params are m_countryFile and m_version.
  string m_directory;
  CountryFile m_countryFile;
  int64_t m_version;

  MapOptions m_files;

  /// Size of file which contains map section in bytes. It's mwm file in any case.
  uint64_t m_mapSize;
  /// Size of file which contains routing section in bytes.
  /// It's .mwm.routing file in case of big (two component) mwms.
  /// And m_routingSize == 0 for small (one compontent) mwms.
  uint64_t m_routingSize;
};

string DebugPrint(LocalCountryFile const & file);
}  // namespace platform
