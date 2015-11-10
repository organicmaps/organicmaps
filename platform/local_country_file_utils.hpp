#pragma once

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class ModelReader;

namespace platform
{
// Removes all files downloader creates during downloading of a country.
void DeleteDownloaderFilesForCountry(CountryFile const & countryFile, int64_t version);

// Finds all local map files in |directory|. Version of these files is
// passed as an argument. Also, performs cleanup described in comment
// for FindAllLocalMapsAndCleanup().
void FindAllLocalMapsInDirectoryAndCleanup(string const & directory, int64_t version,
                                           int64_t latestVersion,
                                           vector<LocalCountryFile> & localFiles);

// Finds all local map files in resources and writable directory. For
// Android, checks /Android/obb directory.  Subdirectories in the
// writable directory should have the following structure:
//
// dir/*.mwm            -- map files, base name should correspond to countries.txt,
//                      -- version is assumed to be zero (unknown).
// dir/*.mwm.routing    -- routing files for corresponding map files,
//                      -- version is assumed to be zero (unknown).
// dir/[0-9]+/*.mwm     -- map files, base name should correspond to countries.txt,
//                      -- version is assumed to be the name of a directory.
// dir/[0-9]{1,18}/*.mwm.routing  -- routing file for corresponding map files,
//                                -- version is assumed to be the name of a directory.
//
// Also, this method performs cleanup described in a comment for
// CleanupMapsDirectory().
void FindAllLocalMapsAndCleanup(int64_t latestVersion, vector<LocalCountryFile> & localFiles);

// This method removes:
// * partially downloaded non-latest maps (with version less than |latestVersion|)
// * empty directories
// * old (format v1) maps
// * old (split) Japan and Brazil maps
// * indexes for absent countries
void CleanupMapsDirectory(int64_t latestVersion);

// Tries to parse a version from a string of size not longer than 18
// symbols and representing an unsigned decimal number. Leading zeroes
// are allowed.
bool ParseVersion(string const & s, int64_t & version);

// When version is zero, uses writable directory, otherwise, creates
// directory with name equal to decimal representation of version.
shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(CountryFile const & countryFile,
                                                         int64_t version);

string GetFileDownloadPath(CountryFile const & countryFile, MapOptions file, int64_t version);

ModelReader * GetCountryReader(LocalCountryFile const & file, MapOptions options);

// An API for managing country indexes.
class CountryIndexes
{
public:
  enum class Index
  {
    Bits,
    Nodes,
    Offsets
  };

  /// Prepares (if necessary) directory for country indexes. Local file
  /// should point to existing local country files.
  /// @throw FileSystemException if any file system error occured.
  static void PreparePlaceOnDisk(LocalCountryFile const & localFile);

  // Removes country indexes from disk including containing directory.
  static bool DeleteFromDisk(LocalCountryFile const & localFile);

  // Returns full path to country index. Note that this method does
  // not create a file on disk - it just returns a path where the
  // index should be created/accessed/removed.
  static string GetPath(LocalCountryFile const & localFile, Index index);

  // Pushes to the exts's end possible index files extensions.
  static void GetIndexesExts(vector<string> & exts);

  // Returns true if |file| corresponds to an index file.
  static bool IsIndexFile(string const & file);

private:
  friend void UnitTest_LocalCountryFile_CountryIndexes();
  friend void UnitTest_LocalCountryFile_DoNotDeleteUserFiles();

  static string IndexesDir(LocalCountryFile const & localFile);
};

string DebugPrint(CountryIndexes::Index index);
}  // namespace platform
