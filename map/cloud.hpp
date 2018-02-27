#pragma once

#include "map/user.hpp"

#include "coding/serdes_json.hpp"

#include "base/assert.hpp"
#include "base/visitor.hpp"

#include <functional>
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
      : m_name(name)
      , m_sizeInBytes(sizeInBytes)
      , m_isOutdated(isOutdated)
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

    DECLARE_VISITOR_AND_DEBUG_PRINT(Index, visitor(m_entries, "entries"),
                                    visitor(m_lastUpdateInHours, "lastUpdateInHours"),
                                    visitor(m_isOutdated, "isOutdated"),
                                    visitor(m_lastSyncTimestamp, "lastSyncTimestamp"))
  };

  struct UploadingRequestData
  {
    std::string m_alohaId;
    std::string m_deviceName;
    std::string m_fileName;

    DECLARE_VISITOR_AND_DEBUG_PRINT(UploadingRequestData, visitor(m_alohaId, "aloha_id"),
                                    visitor(m_deviceName, "device_name"),
                                    visitor(m_fileName, "file_name"))
  };

  struct UploadingResponseData
  {
    std::string m_url;
    std::vector<std::vector<std::string>> m_fields;
    std::string m_method;

    DECLARE_VISITOR_AND_DEBUG_PRINT(UploadingResponseData, visitor(m_url, "url"),
                                    visitor(m_fields, "fields"),
                                    visitor(m_method, "method"))
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

    RequestStatus m_status = RequestStatus::Ok;
    std::string m_error;
  };

  struct UploadingResult
  {
    RequestResult m_requestResult;
    UploadingResponseData m_response;
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

  enum class SynchronizationResult
  {
    // Synchronization was finished successfully.
    Success = 0,
    // Synchronization was interrupted by an authentication error.
    AuthError = 1,
    // Synchronization was interrupted by a network error.
    NetworkError = 2,
    // Synchronization was interrupted by a disk error.
    DiskError = 3
  };

  using InvalidTokenHandler = std::function<void()>;
  using SynchronizationStartedHandler = std::function<void()>;
  using SynchronizationFinishedHandler = std::function<void(SynchronizationResult,
                                                            std::string const & error)>;

  struct CloudParams
  {
    CloudParams() = default;
    CloudParams(std::string && indexName, std::string && serverPathName,
                std::string && settingsParamName, std::string && zipExtension)
      : m_indexName(std::move(indexName))
      , m_serverPathName(std::move(serverPathName))
      , m_settingsParamName(std::move(settingsParamName))
      , m_zipExtension(std::move(zipExtension))
    {}

    // Name of file in which cloud stores metadata.
    std::string m_indexName;
    // Part of path to the cloud server.
    std::string m_serverPathName;
    // Name of parameter to store cloud's state in settings.
    std::string m_settingsParamName;
    // Extension of zipped file. The first character must be '.'
    std::string m_zipExtension;
  };

  explicit Cloud(CloudParams && params);
  ~Cloud();

  void SetInvalidTokenHandler(InvalidTokenHandler && onInvalidToken);
  void SetSynchronizationHandlers(SynchronizationStartedHandler && onSynchronizationStarted,
                                  SynchronizationFinishedHandler && onSynchronizationFinished);

  void SetState(State state);
  State GetState() const;
  // Return timestamp of the last synchronization in milliseconds.
  uint64_t GetLastSynchronizationTimestampInMs() const;

  void Init(std::vector<std::string> const & filePaths);

  std::unique_ptr<User::Subscriber> GetUserSubscriber();

private:
  void LoadIndex();
  bool ReadIndex();
  void UpdateIndex(bool indexExists);
  void SaveIndexImpl() const;

  EntryPtr GetEntryImpl(std::string const & fileName) const;
  void MarkModifiedImpl(std::string const & filePath);

  uint64_t CalculateUploadingSizeImpl() const;
  void SortEntriesBeforeUploadingImpl();
  void ScheduleUploading();
  void ScheduleUploadingTask(EntryPtr const & entry, uint32_t timeout,
                             uint32_t attemptIndex);
  EntryPtr FindOutdatedEntry() const;
  void FinishUploading(SynchronizationResult result, std::string const & errorStr);
  void SetAccessToken(std::string const & token);

  // This function always returns path to a temporary file or the empty string
  // in case of a disk error.
  std::string PrepareFileToUploading(std::string const & fileName);

  UploadingResult RequestUploading(std::string const & uploadingUrl,
                                   std::string const & filePath) const;
  RequestResult ExecuteUploading(UploadingResponseData const & responseData,
                                 std::string const & filePath);
  RequestResult NotifyAboutUploading(std::string const & notificationUrl,
                                     std::string const & filePath) const;

  CloudParams const m_params;
  InvalidTokenHandler m_onInvalidToken;
  SynchronizationStartedHandler m_onSynchronizationStarted;
  SynchronizationFinishedHandler m_onSynchronizationFinished;
  State m_state;
  Index m_index;
  std::string m_accessToken;
  std::map<std::string, std::string> m_files;
  bool m_uploadingStarted = false;
  bool m_indexUpdated = false;
  mutable std::mutex m_mutex;
};
