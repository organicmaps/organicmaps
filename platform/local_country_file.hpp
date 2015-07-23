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
  LocalCountryFile(string const & directory, CountryFile const & countryFile, int64_t version);

  // Syncs internal state like availability of map and routing files,
  // their sizes etc. with disk.
  void SyncWithDisk();

  // Removes specified files from disk if they're known for LocalCountryFile, i.e.
  // were found by previous SyncWithDisk() call.
  void DeleteFromDisk(TMapOptions files) const;

  // Returns path to a file. Return value may be empty until
  // SyncWithDisk() is called.
  string GetPath(TMapOptions file) const;

  // Returns size of a file. Return value may be zero until
  // SyncWithDisk() is called.
  uint32_t GetSize(TMapOptions filesMask) const;

  // Returns a mask of all known country files. Return value may be
  // empty until SyncWithDisk() is called.
  inline TMapOptions GetFiles() const { return m_files; }

  // Checks whether files specified in filesMask are on disk. Return
  // value will be false until SyncWithDisk() is called.
  inline bool OnDisk(TMapOptions filesMask) const
  {
    return (static_cast<unsigned>(m_files) & static_cast<unsigned>(filesMask)) ==
           static_cast<unsigned>(filesMask);
  }

  inline string const & GetDirectory() const { return m_directory; }
  inline string GetCountryName() const { return GetCountryFile().GetNameWithoutExt(); }
  inline int64_t GetVersion() const { return m_version; }
  inline CountryFile const & GetCountryFile() const { return m_countryFile; }

  bool operator<(LocalCountryFile const & rhs) const;
  bool operator==(LocalCountryFile const & rhs) const;

  // Creates LocalCountryFile for test purposes, for a country region
  // with countryFileName (without any extensions). Automatically
  // performs sync with disk.
  static LocalCountryFile MakeForTesting(string const & countryFileName);

  /// @todo The easiest solution for now. Need to be removed in future.
  /// @param fullPath Full path to the mwm file.
  static LocalCountryFile MakeTemporary(string const & fullPath);

private:
  friend string DebugPrint(LocalCountryFile const &);
  friend void UnitTest_LocalCountryFile_DirectoryLookup();
  friend void FindAllLocalMaps(vector<LocalCountryFile> & localFiles);

  string m_directory;
  CountryFile m_countryFile;
  int64_t m_version;
  TMapOptions m_files;

  uint64_t m_mapSize;
  uint64_t m_routingSize;
};

string DebugPrint(LocalCountryFile const & file);
}  // namespace platform
