#pragma once

#include "platform/country_defines.hpp"
#include "platform/gui_thread.hpp"
#include "platform/marketing_service.hpp"
#include "platform/secure_storage.hpp"

#include "coding/reader.hpp"

#include "base/exception.hpp"
#include "base/macros.hpp"
#include "base/task_loop.hpp"
#include "base/worker_thread.hpp"

#include "std/bitset.hpp"
#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "defines.hpp"

DECLARE_EXCEPTION(FileAbsentException, RootException);
DECLARE_EXCEPTION(FileSystemException, RootException);

namespace platform
{
class LocalCountryFile;
}

class Platform
{
public:
  enum EError
  {
    ERR_OK = 0,
    ERR_FILE_DOES_NOT_EXIST,
    ERR_ACCESS_FAILED,
    ERR_DIRECTORY_NOT_EMPTY,
    ERR_FILE_ALREADY_EXISTS,
    ERR_NAME_TOO_LONG,
    ERR_NOT_A_DIRECTORY,
    ERR_SYMLINK_LOOP,
    ERR_IO_ERROR,
    ERR_UNKNOWN
  };

  enum EFileType
  {
    FILE_TYPE_UNKNOWN = 0x1,
    FILE_TYPE_REGULAR = 0x2,
    FILE_TYPE_DIRECTORY = 0x4
  };

  enum class EConnectionType : uint8_t
  {
    CONNECTION_NONE,
    CONNECTION_WIFI,
    CONNECTION_WWAN
  };

  enum class ChargingStatus : uint8_t
  {
    Unknown,
    Plugged,
    Unplugged
  };

  using TFilesWithType = vector<pair<string, EFileType>>;

protected:
  /// Usually read-only directory for application resources
  string m_resourcesDir;
  /// Writable directory to store downloaded map data
  /// @note on some systems it can point to external ejectable storage
  string m_writableDir;
  /// Temporary directory, can be cleaned up by the system
  string m_tmpDir;
  /// Writable directory to store persistent application data
  string m_settingsDir;

  /// Extended resource files.
  /// Used in Android only (downloaded zip files as a container).
  vector<string> m_extResFiles;
  /// Default search scope for resource files.
  /// Used in Android only and initialized according to the market type (Play, Amazon, Samsung).
  string m_androidDefResScope;

  /// Used in Android only to get corret GUI elements layout.
  bool m_isTablet;

  /// Returns last system call error as EError.
  static EError ErrnoToError();

  /// Platform-dependent marketing services.
  MarketingService m_marketingService;

  /// Platform-dependent secure storage.
  platform::SecureStorage m_secureStorage;

  unique_ptr<base::TaskLoop> m_guiThread;

  base::WorkerThread m_networkThread;

public:
  Platform();
  virtual ~Platform() = default;

  static bool IsFileExistsByFullPath(string const & filePath);
  static void DisableBackupForFile(string const & filePath);

  /// @return true if we can create custom texture allocator in drape
  static bool IsCustomTextureAllocatorSupported();
  /// @return always the same writable dir for current user with slash at the end
  string WritableDir() const { return m_writableDir; }
  /// Set writable dir — use for testing and linux stuff only
  void SetWritableDirForTests(string const & path);
  /// @return full path to file in user's writable directory
  string WritablePathForFile(string const & file) const { return WritableDir() + file; }
  /// Uses m_writeableDir [w], m_resourcesDir [r], m_settingsDir [s].
  string ReadPathForFile(string const & file, string searchScope = string()) const;

  /// @return resource dir (on some platforms it's differ from Writable dir)
  string ResourcesDir() const { return m_resourcesDir; }
  /// @note! This function is used in generator_tool and unit tests.
  /// Client app should not replace default resource dir.
  void SetResourceDir(string const & path);

  /// Creates the directory in the filesystem.
  WARN_UNUSED_RESULT static EError MkDir(string const & dirName);

  /// Creates the directory. Returns true on success.
  /// Returns false and logs the reason on failure.
  WARN_UNUSED_RESULT static bool MkDirChecked(string const & dirName);

  /// Removes empty directory from the filesystem.
  static EError RmDir(string const & dirName);

  /// Removes directory from the filesystem.
  /// @note Directory can be non empty.
  /// @note If function fails, directory can be partially removed.
  static bool RmDirRecursively(string const & dirName);

  /// @return path for directory with temporary files with slash at the end
  string TmpDir() const { return m_tmpDir; }
  /// @return full path to file in the temporary directory
  string TmpPathForFile(string const & file) const { return TmpDir() + file; }

  /// @return full path to the file where data for unit tests is stored.
  string TestsDataPathForFile(string const & file) const { return ReadPathForFile(file); }

  /// @return path for directory in the persistent memory, can be the same
  /// as WritableDir, but on some platforms it's different
  string SettingsDir() const { return m_settingsDir; }
  /// Set settings dir — use for testing.
  void SetSettingsDirForTests(string const & path);
  /// @return full path to file in the settings directory
  string SettingsPathForFile(string const & file) const { return SettingsDir() + file; }

  /// @return reader for file decriptor.
  /// @throws FileAbsentException
  /// @param[in] file name or full path which we want to read
  /// @param[in] searchScope looks for file in dirs in given order: \n
  /// [w]ritable, [r]esources, [s]ettings, by [f]ull path, [e]xternal resources,
  unique_ptr<ModelReader>
  GetReader(string const & file, string const & searchScope = string()) const;

  /// @name File operations
  //@{
  typedef vector<string> FilesList;
  /// Retrieves files list contained in given directory
  /// @param directory directory path with slash at the end
  //@{
  /// @param ext files extension to find, like ".mwm".
  static void GetFilesByExt(string const & directory, string const & ext, FilesList & outFiles);
  static void GetFilesByRegExp(string const & directory, string const & regexp, FilesList & outFiles);
  //@}

  static void GetFilesByType(string const & directory, unsigned typeMask,
                             TFilesWithType & outFiles);

  static void GetFilesRecursively(string const & directory, FilesList & filesList);

  static bool IsDirectoryEmpty(string const & directory);
  // Returns true if |path| refers to a directory. Returns false otherwise or on error.
  static bool IsDirectory(string const & path);

  static EError GetFileType(string const & path, EFileType & type);

  /// @return false if file is not exist
  /// @note Check files in Writable dir first, and in ReadDir if not exist in Writable dir
  bool GetFileSizeByName(string const & fileName, uint64_t & size) const;
  /// @return false if file is not exist
  /// @note Try do not use in client production code
  static bool GetFileSizeByFullPath(string const & filePath, uint64_t & size);
  //@}

  /// Used to check available free storage space for downloading.
  enum TStorageStatus
  {
    STORAGE_OK = 0,
    STORAGE_DISCONNECTED,
    NOT_ENOUGH_SPACE
  };
  TStorageStatus GetWritableStorageStatus(uint64_t neededSize) const;
  uint64_t GetWritableStorageSpace() const;

  /// @name Functions for concurrent tasks.
  //@{
  void RunOnGuiThread(base::TaskLoop::Task && task);
  void RunOnGuiThread(base::TaskLoop::Task const & task);

  template <typename Task>
  void RunOnNetworkThread(Task && task) { m_networkThread.Push(forward<Task>(task)); }

  enum Priority
  {
    EPriorityBackground,
    EPriorityLow,
    EPriorityDefault,
    EPriorityHigh
  };
  using TFunctor = function<void()>;
  void RunAsync(TFunctor const & fn, Priority p = EPriorityDefault);
  //@}

  // Please note, that number of active cores can vary at runtime.
  // DO NOT assume for the same return value between calls.
  unsigned CpuCores() const;

  void GetFontNames(FilesList & res) const;

  int VideoMemoryLimit() const;

  int PreCachingDepth() const;

  string DeviceName() const;

  string UniqueClientId() const;

  /// @return url for clients to download maps
  //@{
  string MetaServerUrl() const;
  string ResourcesMetaServerUrl() const;
  //@}

  /// @return JSON-encoded list of urls if metaserver is unreachable
  string DefaultUrlsJSON() const;

  bool IsTablet() const { return m_isTablet; }

  /// @return information about kinds of memory which are relevant for a platform.
  /// This method is implemented for iOS and Android only.
  /// @TODO Add implementation
  string GetMemoryInfo() const;

  static EConnectionType ConnectionStatus();
  static bool IsConnected() { return ConnectionStatus() != EConnectionType::CONNECTION_NONE; }

  static ChargingStatus GetChargingStatus();

  void SetupMeasurementSystem() const;

  MarketingService & GetMarketingService() { return m_marketingService; }
  platform::SecureStorage & GetSecureStorage() { return m_secureStorage; }

  // Use this method for testing purposes only.
  void SetGuiThread(unique_ptr<base::TaskLoop> guiThread);

private:
  void GetSystemFontNames(FilesList & res) const;
};

extern Platform & GetPlatform();

string DebugPrint(Platform::EError err);
string DebugPrint(Platform::ChargingStatus status);
