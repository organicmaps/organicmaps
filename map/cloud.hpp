#pragma once

#include "map/user.hpp"

#include "coding/serdes_json.hpp"

#include "base/assert.hpp"
#include "base/visitor.hpp"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class Cloud
{
public:
  struct Entry
  {
    Entry() = default;

    Entry(std::string const & name, uint64_t sizeInBytes, bool isOutdated)
      : m_name(name), m_sizeInBytes(sizeInBytes), m_isOutdated(isOutdated)
    {}

    Entry(std::string const & name, uint64_t sizeInBytes, bool isOutdated, std::string const & hash)
      : m_name(name), m_sizeInBytes(sizeInBytes), m_hash(hash), m_isOutdated(isOutdated)
    {}

    bool operator==(Entry const & entry) const
    {
      return m_name == entry.m_name && m_sizeInBytes == entry.m_sizeInBytes &&
             m_isOutdated == entry.m_isOutdated;
    }

    bool operator!=(Entry const & entry) const
    {
      return !operator==(entry);
    }

    std::string m_name;
    uint64_t m_sizeInBytes = 0;
    std::string m_hash;
    bool m_isOutdated = false;

    DECLARE_VISITOR_AND_DEBUG_PRINT(Entry, visitor(m_name, "name"),
                                    visitor(m_sizeInBytes, "sizeInBytes"),
                                    visitor(m_hash, "hash"),
                                    visitor(m_isOutdated, "isOutdated"))
  };

  using EntryPtr = std::shared_ptr<Entry>;

  struct Index
  {
    std::vector<EntryPtr> m_entries;
    uint64_t m_lastUpdateInHours = 0;
    bool m_isOutdated = false;
    uint64_t m_lastSyncTimestamp = 0; // in seconds.

    bool CanBeUploaded() const { return m_isOutdated && !m_entries.empty(); }

    DECLARE_VISITOR_AND_DEBUG_PRINT(Index, visitor(m_entries, "entries"),
                                    visitor(m_lastUpdateInHours, "lastUpdateInHours"),
                                    visitor(m_isOutdated, "isOutdated"),
                                    visitor(m_lastSyncTimestamp, "lastSyncTimestamp"))
  };

  struct UploadingResponseData
  {
    std::string m_url;
    std::string m_fallbackUrl;
    std::vector<std::vector<std::string>> m_fields;
    std::string m_method;

    DECLARE_VISITOR_AND_DEBUG_PRINT(UploadingResponseData, visitor(m_url, "url"),
                                    visitor(m_fallbackUrl, "fallback_url"),
                                    visitor(m_fields, "fields"),
                                    visitor(m_method, "method"))
  };

  struct SnapshotFileData
  {
    std::string m_fileName;
    uint64_t m_fileSize = 0;
    uint64_t m_datetime = 0;

    DECLARE_VISITOR_AND_DEBUG_PRINT(SnapshotFileData,
                                    visitor(m_fileName, "file_name"),
                                    visitor(m_fileSize, "file_size"),
                                    visitor(m_datetime, "datetime"))
  };

  struct SnapshotResponseData
  {
    std::string m_deviceId;
    std::string m_deviceName;
    uint64_t m_datetime = 0;
    std::vector<SnapshotFileData> m_files;

    uint64_t GetTotalSizeOfFiles() const
    {
      uint64_t sz = 0;
      for (auto const & f : m_files)
        sz += f.m_fileSize;
      return sz;
    }

    DECLARE_VISITOR_AND_DEBUG_PRINT(SnapshotResponseData,
                                    visitor(m_deviceId, "device_id"),
                                    visitor(m_deviceName, "device_name"),
                                    visitor(m_datetime, "datetime"),
                                    visitor(m_files, "files"))
  };

  enum class RequestStatus
  {
    Ok,
    Forbidden,
    NetworkError
  };

  struct RequestResult
  {
    RequestResult() = default;
    RequestResult(RequestStatus status, std::string const & error)
      : m_status(status)
      , m_error(error)
    {}
    
    operator bool() const
    {
      return m_status == RequestStatus::Ok;
    }

    RequestStatus m_status = RequestStatus::Ok;
    std::string m_error;
  };

  struct UploadingResult
  {
    RequestResult m_requestResult;
    bool m_isMalformed = false;
    UploadingResponseData m_response;
  };

  struct SnapshotResult
  {
    RequestResult m_requestResult;
    bool m_isMalformed = false;
    SnapshotResponseData m_response;
  };

  enum class State
  {
    // User never enabled or disabled synchronization via cloud. It is a default state.
    Unknown = 0,
    // User explicitly disabled synchronization via cloud.
    Disabled = 1,
    // User explicitly enabled synchronization via cloud.
    Enabled = 2
  };

  enum class SynchronizationType
  {
    Backup = 0,
    Restore = 1
  };

  enum class SynchronizationResult
  {
    // Synchronization was finished successfully.
    Success = 0,
    // Synchronization was interrupted by an authentication error.
    AuthError = 1,
    // Synchronization was interrupted by a network error.
    NetworkError = 2,
    // Synchronization was interrupted by a disk error.
    DiskError = 3,
    // Synchronization was interrupted by the user.
    UserInterrupted = 4,
    // Synchronization was finished because of invalid function call.
    InvalidCall = 5
  };

  enum class RestoringRequestResult
  {
    // There is a backup on the server.
    BackupExists = 0,
    // There is no backup on the server.
    NoBackup = 1,
    // Not enough space on the disk for the restoring.
    NotEnoughDiskSpace = 2
  };

  struct ConvertionResult
  {
    bool m_isSuccessful = false;
    std::string m_hash;
  };

  using InvalidTokenHandler = std::function<void()>;
  using FileConverter = std::function<ConvertionResult(std::string const & filePath,
                                                       std::string const & convertedFilePath)>;
  using SynchronizationStartedHandler = std::function<void(SynchronizationType)>;
  using SynchronizationFinishedHandler = std::function<void(SynchronizationType,
                                                            SynchronizationResult,
                                                            std::string const & error)>;
  using RestoreRequestedHandler = std::function<void(RestoringRequestResult,
                                                     std::string const & deviceName,
                                                     uint64_t backupTimestampInMs)>;
  using RestoredFilesPreparedHandler = std::function<void()>;

  struct CloudParams
  {
    CloudParams() = default;
    CloudParams(std::string && indexName, std::string && serverPathName,
                std::string && settingsParamName, std::string && restoringFolder,
                std::string && restoredFileExtension,
                FileConverter && backupConverter,
                FileConverter && restoreConverter)
      : m_indexName(std::move(indexName))
      , m_serverPathName(std::move(serverPathName))
      , m_settingsParamName(std::move(settingsParamName))
      , m_restoredFileExtension(std::move(restoredFileExtension))
      , m_restoringFolder(std::move(restoringFolder))
      , m_backupConverter(std::move(backupConverter))
      , m_restoreConverter(std::move(restoreConverter))
    {}

    // Name of file in which cloud stores metadata.
    std::string m_indexName;
    // Part of path to the cloud server.
    std::string m_serverPathName;
    // Name of parameter to store cloud's state in settings.
    std::string m_settingsParamName;
    // The extension of restored files.
    std::string m_restoredFileExtension;
    // The folder in which files will be restored.
    std::string m_restoringFolder;
    // This file converter is executed before uploading to the cloud.
    FileConverter m_backupConverter;
    // This file converter is executed after downloading from the cloud.
    FileConverter m_restoreConverter;
  };

  explicit Cloud(CloudParams && params);
  ~Cloud();

  // Handler can be called from non-UI thread.
  void SetInvalidTokenHandler(InvalidTokenHandler && onInvalidToken);
  // Handlers can be called from non-UI thread except of ApplyRestoredFilesHandler.
  // ApplyRestoredFilesHandler is always called from UI-thread.
  void SetSynchronizationHandlers(SynchronizationStartedHandler && onSynchronizationStarted,
                                  SynchronizationFinishedHandler && onSynchronizationFinished,
                                  RestoreRequestedHandler && onRestoreRequested,
                                  RestoredFilesPreparedHandler && onRestoredFilesPrepared);

  void SetState(State state);
  State GetState() const;
  // Return timestamp of the last synchronization in milliseconds.
  uint64_t GetLastSynchronizationTimestampInMs() const;

  void Init(std::vector<std::string> const & filePaths);

  std::unique_ptr<User::Subscriber> GetUserSubscriber();

  // This function requests restoring of files. The function must be called only
  // if internet connection exists, the cloud is enabled and initialized, user is
  // authenticated and there is no any current restoring process, otherwise
  // InvalidCall result will be got.
  void RequestRestoring();
  
  // This function applies requested files. The function must be called only
  // if internet connection exists, the cloud is enabled and initialized, user is
  // authenticated, there is an existing restoring process and there is
  // the backup on the server, otherwise InvalidCall result will be got.
  void ApplyRestoring();
  
  void CancelRestoring();

  static State GetCloudState(std::string const & paramName);

private:
  struct RestoredFile
  {
    std::string m_filename;
    std::string m_hash;

    RestoredFile() = default;
    RestoredFile(std::string const & filename, std::string const & hash)
      : m_filename(filename), m_hash(hash)
    {}
  };
  using RestoredFilesCollection = std::vector<RestoredFile>;

  void LoadIndex();
  bool ReadIndex();
  void UpdateIndex(bool indexExists);
  void SaveIndexImpl() const;
  
  bool IsRestoringEnabledCommonImpl(std::string & reason) const;
  bool IsRequestRestoringEnabled(std::string & reason) const;
  bool IsApplyRestoringEnabled(std::string & reason) const;

  EntryPtr GetEntryImpl(std::string const & fileName) const;
  void MarkModifiedImpl(std::string const & filePath, bool isOutdated);
  void UpdateIndexByRestoredFilesImpl(RestoredFilesCollection const & files,
                                      uint64_t lastSyncTimestampInSec);

  uint64_t CalculateUploadingSizeImpl() const;
  bool CanUploadImpl() const;
  void SortEntriesBeforeUploadingImpl();
  void ScheduleUploading();
  void ScheduleUploadingTask(EntryPtr const & entry, uint32_t timeout);
  bool UploadFile(std::string const & uploadedName);
  void FinishUploadingOnRequestError(Cloud::RequestResult const & result);
  EntryPtr FindOutdatedEntry() const;
  void FinishUploading(SynchronizationResult result, std::string const & errorStr);
  void SetAccessToken(std::string const & token);
  std::string GetAccessToken() const;

  // This function always returns path to a temporary file or the empty string
  // in case of a disk error.
  std::string PrepareFileToUploading(std::string const & fileName, std::string & hash);

  RequestResult CreateSnapshot(std::vector<std::string> const & files) const;
  RequestResult FinishSnapshot() const;
  SnapshotResult GetBestSnapshot() const;
  void ProcessSuccessfulSnapshot(SnapshotResult const & result);
  UploadingResult RequestUploading(std::string const & filePath) const;
  RequestResult ExecuteUploading(UploadingResponseData const & responseData,
                                 std::string const & filePath);
  RequestResult NotifyAboutUploading(std::string const & filePath, uint64_t fileSize) const;

  void GetBestSnapshotTask(uint32_t timeout, uint32_t attemptIndex);
  void FinishRestoring(SynchronizationResult result, std::string const & errorStr);
  std::list<SnapshotFileData> GetDownloadingList(std::string const & restoringDirPath);
  void DownloadingTask(std::string const & dirPath, bool useFallbackUrl,
                       std::list<SnapshotFileData> && files);
  void CompleteRestoring(std::string const & dirPath);

  void ApplyRestoredFiles(std::string const & dirPath, RestoredFilesCollection && files);

  template <typename HandlerType, typename HandlerGetterType, typename... HandlerArgs>
  void ThreadSafeCallback(HandlerGetterType && handlerGetter, HandlerArgs... handlerArgs)
  {
    HandlerType handler;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      handler = handlerGetter();
    }
    if (handler != nullptr)
      handler(handlerArgs...);
  }

  CloudParams const m_params;
  InvalidTokenHandler m_onInvalidToken;
  SynchronizationStartedHandler m_onSynchronizationStarted;
  SynchronizationFinishedHandler m_onSynchronizationFinished;
  RestoreRequestedHandler m_onRestoreRequested;
  RestoredFilesPreparedHandler m_onRestoredFilesPrepared;
  State m_state;
  Index m_index;
  std::string m_accessToken;
  std::map<std::string, std::string> m_files;
  bool m_uploadingStarted = false;
  std::vector<std::string> m_snapshotFiles;
  bool m_isSnapshotCreated = false;

  enum RestoringState
  {
    None,
    Requested,
    Applying,
    Finalizing
  };
  RestoringState m_restoringState = RestoringState::None;
  bool m_indexUpdated = false;
  SnapshotResponseData m_bestSnapshotData;
  mutable std::mutex m_mutex;
};

inline std::string DebugPrint(Cloud::SynchronizationType type)
{
  switch (type)
  {
  case Cloud::SynchronizationType::Backup: return "Backup";
  case Cloud::SynchronizationType::Restore: return "Restore";
  }
  UNREACHABLE();
}

inline std::string DebugPrint(Cloud::SynchronizationResult result)
{
  switch (result)
  {
  case Cloud::SynchronizationResult::Success: return "Success";
  case Cloud::SynchronizationResult::AuthError: return "AuthError";
  case Cloud::SynchronizationResult::NetworkError: return "NetworkError";
  case Cloud::SynchronizationResult::DiskError: return "DiskError";
  case Cloud::SynchronizationResult::UserInterrupted: return "UserInterrupted";
  case Cloud::SynchronizationResult::InvalidCall: return "InvalidCall";
  }
  UNREACHABLE();
}

inline std::string DebugPrint(Cloud::RestoringRequestResult result)
{
  switch (result)
  {
  case Cloud::RestoringRequestResult::BackupExists: return "BackupExists";
  case Cloud::RestoringRequestResult::NoBackup: return "NoBackup";
  case Cloud::RestoringRequestResult::NotEnoughDiskSpace: return "NotEnoughDiskSpace";
  }
  UNREACHABLE();
}
