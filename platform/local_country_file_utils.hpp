#pragma once

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class ModelReader;

namespace platform
{
// Removes all files downloader creates during downloading of a country.
// Note. The the maps are deleted from writable dir/|dataDir|/|version| directory.
// If |dataDir| is empty (or is not set) the function deletes maps from writable dir.
void DeleteDownloaderFilesForCountry(int64_t version, CountryFile const & countryFile);
void DeleteDownloaderFilesForCountry(int64_t version, std::string const & dataDir,
                                     CountryFile const & countryFile);

// Finds all local map files in |directory|. Version of these files is
// passed as an argument. Also, performs cleanup described in comment
// for FindAllLocalMapsAndCleanup().
void FindAllLocalMapsInDirectoryAndCleanup(std::string const & directory, int64_t version,
                                           int64_t latestVersion,
                                           std::vector<LocalCountryFile> & localFiles);

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
// Note. The the maps are looked for writable dir/|dataDir|/|version| directory.
// If |dataDir| is empty (or is not set) the function looks for maps in writable dir.
void FindAllLocalMapsAndCleanup(int64_t latestVersion, std::vector<LocalCountryFile> & localFiles);
void FindAllLocalMapsAndCleanup(int64_t latestVersion, std::string const & dataDir,
                                std::vector<LocalCountryFile> & localFiles);

void FindAllDiffs(std::string const & dataDir, std::vector<LocalCountryFile> & diffs);

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
bool ParseVersion(std::string const & s, int64_t & version);

// When version is zero, uses writable directory, otherwise, creates
// directory with name equal to decimal representation of version.
// Note. The function assumes the maps are located in writable dir/|dataDir|/|version| directory.
// If |dataDir| is empty (or is not set) the function assumes that maps are in writable dir.
std::shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, CountryFile const & countryFile);
std::shared_ptr<LocalCountryFile> PreparePlaceForCountryFiles(int64_t version, std::string const & dataDir,
                                                         CountryFile const & countryFile);

// Note. The function assumes the maps are located in writable dir/|dataDir|/|version| directory.
// If |dataDir| is empty (or is not set) the function assumes that maps are in writable dir.
std::string GetFileDownloadPath(int64_t version, CountryFile const & countryFile, MapFileType type);
std::string GetFileDownloadPath(int64_t version, std::string const & dataDir,
                                CountryFile const & countryFile, MapFileType type);

std::unique_ptr<ModelReader> GetCountryReader(LocalCountryFile const & file, MapFileType type);

/// An API for managing country indexes.
/// Not used now (except tests), but will be usefull for the Terrain index in future.
class CountryIndexes
{
public:
  enum class Index
  {
    Bits,
    Nodes,
    Offsets
  };

  /// Prepares (if necessary) directory for country indexes.
  /// @throw FileSystemException if any file system error occured.
  static void PreparePlaceOnDisk(LocalCountryFile const & localFile);

  // Removes country indexes from disk including containing directory.
  static bool DeleteFromDisk(LocalCountryFile const & localFile);

  // Returns full path to country index. Note that this method does
  // not create a file on disk - it just returns a path where the
  // index should be created/accessed/removed.
  static std::string GetPath(LocalCountryFile const & localFile, Index index);

  // Pushes to the exts's end possible index files extensions.
  static void GetIndexesExts(std::vector<std::string> & exts);

  // Returns true if |file| corresponds to an index file.
  static bool IsIndexFile(std::string const & file);

private:
  friend void UnitTest_LocalCountryFile_CountryIndexes();
  friend void UnitTest_LocalCountryFile_DoNotDeleteUserFiles();

  static std::string IndexesDir(LocalCountryFile const & localFile);
};

std::string DebugPrint(CountryIndexes::Index index);
}  // namespace platform
