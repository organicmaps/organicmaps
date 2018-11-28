#include "map/cloud.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/sha1.hpp"

#include "platform/network_policy.hpp"
#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/remote_file.hpp"
#include "platform/settings.hpp"
#include "platform/http_uploader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "private.h"

using namespace std::chrono;

namespace
{
uint32_t constexpr kTaskTimeoutInSeconds = 1;
uint64_t constexpr kUpdateTimeoutInHours = 24;
uint32_t constexpr kRetryMaxAttempts = 3;
uint32_t constexpr kRetryTimeoutInSeconds = 5;
uint32_t constexpr kRetryDegradationFactor = 2;

uint64_t constexpr kMaxWwanUploadingSizeInBytes = 10 * 1024; // 10Kb
uint64_t constexpr kMaxUploadingFileSizeInBytes = 100 * 1024 * 1024; // 100Mb

uint32_t constexpr kRequestTimeoutInSec = 5;

std::string const kServerUrl = CLOUD_URL;
std::string const kServerVersion = "v1";
std::string const kServerCreateSnapshotMethod = "snapshot/create";
std::string const kServerFinishSnapshotMethod = "snapshot/finish";
std::string const kServerBestSnapshotMethod = "snapshot/best";
std::string const kServerUploadMethod = "file/upload/start";
std::string const kServerNotifyMethod = "file/upload/finish";
std::string const kServerDownloadMethod = "file/download";
std::string const kApplicationJson = "application/json";
std::string const kSnapshotFile = "snapshot.json";

std::string GetIndexFilePath(std::string const & indexName)
{
  return base::JoinPath(GetPlatform().SettingsDir(), indexName);
}

std::string GetRestoringFolder(std::string const & serverPathName)
{
  return base::JoinPath(GetPlatform().TmpDir(), serverPathName + "_restore");
}

std::string BuildMethodUrl(std::string const & serverPathName, std::string const & methodName)
{
  if (kServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kServerUrl << "/"
     << kServerVersion << "/"
     << serverPathName << "/"
     << methodName << "/";
  return ss.str();
}

std::string BuildAuthenticationToken(std::string const & accessToken)
{
  return "Bearer " + accessToken;
}

std::string ExtractFileNameWithoutExtension(std::string const & filePath)
{
  std::string path = filePath;
  base::GetNameFromFullPath(path);
  base::GetNameWithoutExt(path);
  return path;
}

template<typename DataType>
std::string SerializeToJson(DataType const & data)
{
  std::string jsonStr;
  using Sink = MemWriter<std::string>;
  Sink sink(jsonStr);
  coding::SerializerJson<Sink> serializer(sink);
  serializer(data);
  return jsonStr;
}

template<typename DataType>
void DeserializeFromJson(std::string const & jsonStr, DataType & result)
{
  coding::DeserializerJson des(jsonStr);
  des(result);
}

bool IsSuccessfulResultCode(int resultCode)
{
  return resultCode >= 200 && resultCode < 300;
}

std::string GetDeviceName()
{
  return GetPlatform().DeviceName() + " (" + GetPlatform().DeviceModel() + ")";
}

bool CanUpload(uint64_t totalUploadingSize)
{
  auto const status = GetPlatform().ConnectionStatus();
  switch (status)
  {
  case Platform::EConnectionType::CONNECTION_NONE: return false;
  case Platform::EConnectionType::CONNECTION_WIFI: return true;
  case Platform::EConnectionType::CONNECTION_WWAN:
    return platform::GetCurrentNetworkPolicy().CanUse() &&
           totalUploadingSize <= kMaxWwanUploadingSizeInBytes;
  }
  UNREACHABLE();
}

struct SnapshotCreateRequestData
{
  std::string m_deviceId;
  std::string m_deviceName;
  std::vector<std::string> m_fileNames;

  explicit SnapshotCreateRequestData(std::vector<std::string> const & files = {})
    : m_deviceId(GetPlatform().UniqueClientId()), m_deviceName(GetDeviceName()), m_fileNames(files)
  {}

  DECLARE_VISITOR(visitor(m_deviceId, "device_id"), visitor(m_deviceName, "device_name"),
                  visitor(m_fileNames, "file_names"))
};

struct SnapshotRequestData
{
  std::string m_deviceId;

  SnapshotRequestData() : m_deviceId(GetPlatform().UniqueClientId()) {}

  DECLARE_VISITOR(visitor(m_deviceId, "device_id"))
};

struct UploadingRequestData
{
  std::string m_deviceId;
  std::string m_fileName;
  std::string m_locale;

  explicit UploadingRequestData(std::string const & filePath = {})
    : m_deviceId(GetPlatform().UniqueClientId())
    , m_fileName(ExtractFileNameWithoutExtension(filePath))
    , m_locale(languages::GetCurrentOrig())
  {}

  DECLARE_VISITOR(visitor(m_deviceId, "device_id"), visitor(m_fileName, "file_name"),
                  visitor(m_locale, "locale"))
};

struct NotifyRequestData
{
  std::string m_deviceId;
  std::string m_fileName;
  uint64_t m_fileSize = 0;

  NotifyRequestData(std::string const & filePath, uint64_t fileSize)
    : m_deviceId(GetPlatform().UniqueClientId())
    , m_fileName(ExtractFileNameWithoutExtension(filePath))
    , m_fileSize(fileSize)
  {}

  DECLARE_VISITOR(visitor(m_deviceId, "device_id"), visitor(m_fileName, "file_name"),
                  visitor(m_fileSize, "file_size"))
};

struct DownloadingRequestData
{
  std::string m_deviceId;
  std::string m_fileName;
  uint64_t m_datetime = 0;

  DownloadingRequestData(std::string const & deviceId, std::string const & fileName,
                         uint64_t datetime)
    : m_deviceId(deviceId), m_fileName(fileName), m_datetime(datetime)
  {}

  DECLARE_VISITOR(visitor(m_deviceId, "device_id"), visitor(m_fileName, "file_name"),
                  visitor(m_datetime, "datetime"))
};

struct DownloadingResponseData
{
  std::string m_url;
  std::string m_fallbackUrl;

  DECLARE_VISITOR(visitor(m_url, "url"), visitor(m_fallbackUrl, "fallback_url"))
};

struct DownloadingResult
{
  Cloud::RequestResult m_requestResult;
  bool m_isMalformed = false;
  DownloadingResponseData m_response;
};

using ResponseHandler = std::function<Cloud::RequestResult(
  int code, std::string const & response)>;

template <typename RequestDataType, typename... RequestDataArgs>
Cloud::RequestResult CloudRequestWithResult(std::string const & url,
                                            std::string const & accessToken,
                                            ResponseHandler const & responseHandler,
                                            RequestDataArgs const & ... args)
{
  ASSERT(responseHandler != nullptr, ());

  platform::HttpClient request(url);
  request.SetTimeout(kRequestTimeoutInSec);
  request.SetRawHeader("Accept", kApplicationJson);
  request.SetRawHeader("Authorization", BuildAuthenticationToken(accessToken));
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
  request.SetBodyData(SerializeToJson(RequestDataType(args...)), kApplicationJson);

  if (request.RunHttpRequest() && !request.WasRedirected())
  {
    int const resultCode = request.ErrorCode();
    if (resultCode == 403)
    {
      LOG(LWARNING, ("Access denied for", url));
      return {Cloud::RequestStatus::Forbidden, request.ServerResponse()};
    }

    return responseHandler(resultCode, request.ServerResponse());
  }

  return {Cloud::RequestStatus::NetworkError, request.ServerResponse()};
}

template <typename RequestDataType, typename... RequestDataArgs>
Cloud::RequestResult CloudRequest(std::string const & url, std::string const & accessToken,
                                  RequestDataArgs const &... args)
{
  auto responseHandler = [](int code, std::string const & serverResponse) -> Cloud::RequestResult {
    if (IsSuccessfulResultCode(code))
      return {Cloud::RequestStatus::Ok, {}};
    return {Cloud::RequestStatus::NetworkError, serverResponse};
  };

  return CloudRequestWithResult<RequestDataType>(url, accessToken, responseHandler, args...);
}

template <typename ResultType>
void ParseRequestJsonResult(std::string const & url, std::string const & serverResponse,
                            ResultType & result)
{
  try
  {
    DeserializeFromJson(serverResponse, result.m_response);
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Malformed server response", "url:", url, "response:", serverResponse));
    result.m_response = {};
    result.m_isMalformed = true;
  }
}

template <typename RequestDataType, typename ResultType, typename... RequestDataArgs>
ResultType CloudRequestWithJsonResult(std::string const & url, std::string const & accessToken,
                                      RequestDataArgs const &... args)
{
  ResultType result;
  auto responseHandler = [&result, &url](int code,
    std::string const & serverResponse) -> Cloud::RequestResult
  {
    if (IsSuccessfulResultCode(code))
    {
      ParseRequestJsonResult(url, serverResponse, result);
      return {Cloud::RequestStatus::Ok, {}};
    }
    return {Cloud::RequestStatus::NetworkError, serverResponse};
  };
  result.m_requestResult =
      CloudRequestWithResult<RequestDataType>(url, accessToken, responseHandler, args...);
  return result;
}

Cloud::SnapshotResponseData ReadSnapshotFile(std::string const & filename)
{
  if (!GetPlatform().IsFileExistsByFullPath(filename))
    return {};

  std::string jsonStr;
  try
  {
    FileReader r(filename);
    r.ReadAsString(jsonStr);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", filename,
                   "reason:", exception.what()));
    return {};
  }

  if (jsonStr.empty())
    return {};

  try
  {
    Cloud::SnapshotResponseData data;
    DeserializeFromJson(jsonStr, data);
    return data;
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing file:", filename,
                   "reason:", exception.what(), "json:", jsonStr));
  }
  return {};
}

bool CheckAndGetFileSize(std::string const & filePath, uint64_t & fileSize)
{
  if (!base::GetFileSize(filePath, fileSize))
    return false;

  // We do not work with files which size is more than kMaxUploadingFileSizeInBytes.
  return fileSize <=  kMaxUploadingFileSizeInBytes;
}
}  // namespace

Cloud::Cloud(CloudParams && params)
  : m_params(std::move(params))
{
  ASSERT(!m_params.m_indexName.empty(), ());
  ASSERT(!m_params.m_serverPathName.empty(), ());
  ASSERT(!m_params.m_settingsParamName.empty(), ());
  ASSERT(!m_params.m_restoredFileExtension.empty(), ());
  ASSERT(!m_params.m_restoringFolder.empty(), ());

  m_state = GetCloudState(m_params.m_settingsParamName);
  GetPlatform().RunTask(Platform::Thread::File, [this]() { ReadIndex(); });
}

Cloud::~Cloud()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  SaveIndexImpl();
}

void Cloud::SetInvalidTokenHandler(InvalidTokenHandler && onInvalidToken)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_onInvalidToken = std::move(onInvalidToken);
}

void Cloud::SetSynchronizationHandlers(SynchronizationStartedHandler && onSynchronizationStarted,
                                       SynchronizationFinishedHandler && onSynchronizationFinished,
                                       RestoreRequestedHandler && onRestoreRequested,
                                       RestoredFilesPreparedHandler && onRestoredFilesPrepared)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_onSynchronizationStarted = std::move(onSynchronizationStarted);
  m_onSynchronizationFinished = std::move(onSynchronizationFinished);
  m_onRestoreRequested = std::move(onRestoreRequested);
  m_onRestoredFilesPrepared = std::move(onRestoredFilesPrepared);
}

void Cloud::SetState(State state)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_state == state)
    return;

  m_state = state;
  settings::Set(m_params.m_settingsParamName, static_cast<int>(m_state));

  switch (m_state)
  {
  case State::Enabled:
    GetPlatform().RunTask(Platform::Thread::File, [this]() { LoadIndex(); });
    break;

  case State::Disabled:
    // Delete index file and clear memory.
    base::DeleteFileX(GetIndexFilePath(m_params.m_indexName));
    m_index = Index();
    break;

  case State::Unknown: ASSERT(false, ("Unknown state can't be set up")); break;
  }
}

Cloud::State Cloud::GetState() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_state;
}

void Cloud::Init(std::vector<std::string> const & filePaths)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto const & filePath : filePaths)
    m_files[ExtractFileNameWithoutExtension(filePath)] = filePath;

  if (m_state != State::Enabled)
    return;

  GetPlatform().RunTask(Platform::Thread::File, [this]() { LoadIndex(); });
}

uint64_t Cloud::GetLastSynchronizationTimestampInMs() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_state != State::Enabled)
    return 0;

  return m_index.m_lastSyncTimestamp * 1000; // in milliseconds.
}

std::unique_ptr<User::Subscriber> Cloud::GetUserSubscriber()
{
  auto s = std::make_unique<User::Subscriber>();
  s->m_onChangeToken = [this](std::string const & token)
  {
    SetAccessToken(token);
    ScheduleUploading();
  };
  return s;
}

void Cloud::LoadIndex()
{
  UpdateIndex(ReadIndex());
  ScheduleUploading();
}

bool Cloud::ReadIndex()
{
  auto const indexFilePath = GetIndexFilePath(m_params.m_indexName);
  if (!GetPlatform().IsFileExistsByFullPath(indexFilePath))
    return false;

  // Read index file.
  std::string jsonStr;
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(jsonStr);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", indexFilePath,
                   "reason:", exception.what()));
    return false;
  }

  // Parse index file.
  if (jsonStr.empty())
    return false;

  try
  {
    Index index;
    DeserializeFromJson(jsonStr, index);
    std::lock_guard<std::mutex> lock(m_mutex);
    std::swap(m_index, index);
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing file:", indexFilePath,
                   "reason:", exception.what(), "json:", jsonStr));
    return false;
  }
  return true;
}

void Cloud::UpdateIndex(bool indexExists)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Now we process files ONLY if update time is out.
  auto const h = static_cast<uint64_t>(
    duration_cast<hours>(system_clock::now().time_since_epoch()).count());
  if (!indexExists || h >= m_index.m_lastUpdateInHours + kUpdateTimeoutInHours)
  {
    for (auto const & path : m_files)
      MarkModifiedImpl(path.second, true /* isOutdated */);

    // Erase disappeared files from index.
    base::EraseIf(m_index.m_entries, [this](EntryPtr const & entity) {
      return m_files.find(entity->m_name) == m_files.end();
    });

    // Index is outdated only if there is an entry.
    m_index.m_isOutdated = !m_index.m_entries.empty();
    if (m_index.m_isOutdated)
      m_index.m_lastUpdateInHours = h;

    SaveIndexImpl();
  }
  m_indexUpdated = true;
}

uint64_t Cloud::CalculateUploadingSizeImpl() const
{
  uint64_t sz = 0;
  for (auto const & entry : m_index.m_entries)
    sz += entry->m_sizeInBytes;
  return sz;
}

bool Cloud::CanUploadImpl() const
{
  return m_state == State::Enabled && m_index.CanBeUploaded() && !m_accessToken.empty() &&
         !m_uploadingStarted && m_indexUpdated && m_restoringState == RestoringState::None &&
         CanUpload(CalculateUploadingSizeImpl());
}

void Cloud::SortEntriesBeforeUploadingImpl()
{
  std::sort(m_index.m_entries.begin(), m_index.m_entries.end(),
            [](EntryPtr const & lhs, EntryPtr const & rhs)
  {
    return lhs->m_sizeInBytes < rhs->m_sizeInBytes;
  });
}

void Cloud::MarkModifiedImpl(std::string const & filePath, bool isOutdated)
{
  uint64_t fileSize = 0;
  if (!CheckAndGetFileSize(filePath, fileSize))
    return;

  auto const fileName = ExtractFileNameWithoutExtension(filePath);
  auto entryPtr = GetEntryImpl(fileName);
  if (entryPtr)
  {
    entryPtr->m_isOutdated = isOutdated;
    entryPtr->m_sizeInBytes = fileSize;
  }
  else
  {
    m_index.m_entries.emplace_back(
      std::make_shared<Entry>(fileName, fileSize, isOutdated));
  }
}

void Cloud::UpdateIndexByRestoredFilesImpl(RestoredFilesCollection const & files,
                                           uint64_t lastSyncTimestampInSec)
{
  m_index.m_isOutdated = false;
  m_index.m_lastUpdateInHours =
    static_cast<uint64_t>(duration_cast<hours>(system_clock::now().time_since_epoch()).count());
  m_index.m_lastSyncTimestamp = lastSyncTimestampInSec;
  m_index.m_entries.clear();
  for (auto const & f : files)
  {
    uint64_t fileSize = 0;
    if (!CheckAndGetFileSize(f.m_filename, fileSize))
      continue;

    m_index.m_entries.emplace_back(
      std::make_shared<Entry>(ExtractFileNameWithoutExtension(f.m_filename), fileSize,
                              false /* isOutdated */, f.m_hash));
  }
  SaveIndexImpl();
}

Cloud::EntryPtr Cloud::GetEntryImpl(std::string const & fileName) const
{
  auto it = std::find_if(m_index.m_entries.begin(), m_index.m_entries.end(),
                         [&fileName](EntryPtr ptr) { return ptr->m_name == fileName; });
  if (it != m_index.m_entries.end())
    return *it;
  return nullptr;
}

void Cloud::SaveIndexImpl() const
{
  if (m_state != State::Enabled || m_index.m_entries.empty())
    return;

  auto const indexFilePath = GetIndexFilePath(m_params.m_indexName);
  try
  {
    auto jsonData = SerializeToJson(m_index);
    FileWriter w(indexFilePath);
    w.Write(jsonData.c_str(), jsonData.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", indexFilePath,
                   "reason:", exception.what()));
  }
}

void Cloud::ScheduleUploading()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!CanUploadImpl())
      return;

    SortEntriesBeforeUploadingImpl();

    m_snapshotFiles.clear();
    m_snapshotFiles.reserve(m_index.m_entries.size());
    for (auto const & entry : m_index.m_entries)
      m_snapshotFiles.emplace_back(entry->m_name);

    m_uploadingStarted = true;
    m_isSnapshotCreated = false;
  }

  ThreadSafeCallback<SynchronizationStartedHandler>([this]() { return m_onSynchronizationStarted; },
                                                    SynchronizationType::Backup);

  auto entry = FindOutdatedEntry();
  if (entry != nullptr)
  {
    ScheduleUploadingTask(entry, kTaskTimeoutInSeconds);
  }
  else
  {
    ThreadSafeCallback<SynchronizationFinishedHandler>([this]() { return m_onSynchronizationFinished; },
                                                       SynchronizationType::Backup,
                                                       SynchronizationResult::Success, "");
  }
}

void Cloud::ScheduleUploadingTask(EntryPtr const & entry, uint32_t timeout)
{
  GetPlatform().RunDelayedTask(Platform::Thread::Network, seconds(timeout), [this, entry]()
  {
    std::string entryName;
    std::string entryHash;
    bool isInvalidToken;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      // Uploading has finished.
      if (!m_uploadingStarted)
        return;

      ASSERT(entry->m_isOutdated, ());
      entryName = entry->m_name;
      entryHash = entry->m_hash;
      isInvalidToken = m_accessToken.empty();
    }
    
    if (!m_params.m_backupConverter)
    {
      FinishUploading(SynchronizationResult::InvalidCall, "Backup converter is not set");
      return;
    }

    if (kServerUrl.empty())
    {
      FinishUploading(SynchronizationResult::NetworkError, "Empty server url");
      return;
    }

    // Access token may become invalid between tasks.
    if (isInvalidToken)
    {
      FinishUploading(SynchronizationResult::AuthError, "Access token is empty");
      return;
    }

    // Prepare file to uploading.
    std::string hash;
    auto const uploadedName = PrepareFileToUploading(entryName, hash);
    auto deleteAfterUploading = [uploadedName]() {
      if (!uploadedName.empty())
        base::DeleteFileX(uploadedName);
    };
    SCOPE_GUARD(deleteAfterUploadingGuard, deleteAfterUploading);

    if (uploadedName.empty())
    {
      FinishUploading(SynchronizationResult::DiskError, "File preparation error");
      return;
    }

    // Upload only if calculated hash is not equal to previous one.
    if (entryHash != hash && !UploadFile(uploadedName))
      return;

    // Mark entry as not outdated.
    bool isSnapshotCreated;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      entry->m_isOutdated = false;
      entry->m_hash = hash;
      SaveIndexImpl();
      isSnapshotCreated = m_isSnapshotCreated;
    }

    // Schedule next uploading task.
    auto nextEntry = FindOutdatedEntry();
    if (nextEntry != nullptr)
    {
      ScheduleUploadingTask(nextEntry, kTaskTimeoutInSeconds);
      return;
    }

    // Finish snapshot.
    if (isSnapshotCreated)
    {
      auto const result = FinishSnapshot();
      if (!result)
      {
        FinishUploadingOnRequestError(result);
        return;
      }
    }
    FinishUploading(SynchronizationResult::Success, {});
  });
}

bool Cloud::UploadFile(std::string const & uploadedName)
{
  uint64_t uploadedFileSize = 0;
  if (!base::GetFileSize(uploadedName, uploadedFileSize))
  {
    FinishUploading(SynchronizationResult::DiskError, "File size calculation error");
    return false;
  }

  // Create snapshot if it was not created early.
  bool snapshotCreated;
  std::vector<std::string> snapshotFiles;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    snapshotCreated = m_isSnapshotCreated;
    std::swap(snapshotFiles, m_snapshotFiles);
  }
  if (!snapshotCreated)
  {
    auto const result = CreateSnapshot(snapshotFiles);
    if (!result)
    {
      FinishUploadingOnRequestError(result);
      return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_isSnapshotCreated = true;
  }

  // Request uploading.
  auto const result = RequestUploading(uploadedName);
  if (result.m_isMalformed)
  {
    FinishUploading(SynchronizationResult::NetworkError, "Malformed uploading response");
    return false;
  }
  if (!result.m_requestResult)
  {
    FinishUploadingOnRequestError(result.m_requestResult);
    return false;
  }

  // Execute uploading.
  auto const executeResult = ExecuteUploading(result.m_response, uploadedName);
  if (!executeResult)
  {
    FinishUploadingOnRequestError(executeResult);
    return false;
  }

  // Notify about successful uploading.
  auto const notificationResult = NotifyAboutUploading(uploadedName, uploadedFileSize);
  if (!notificationResult)
  {
    FinishUploadingOnRequestError(notificationResult);
    return false;
  }

  return true;
}

void Cloud::FinishUploadingOnRequestError(Cloud::RequestResult const & result)
{
  switch (result.m_status)
  {
  case RequestStatus::Ok:
    ASSERT(false, ("Possibly incorrect call"));
    return;
  case RequestStatus::Forbidden:
    FinishUploading(SynchronizationResult::AuthError, result.m_error);
    return;
  case RequestStatus::NetworkError:
    FinishUploading(SynchronizationResult::NetworkError, result.m_error);
    return;
  }
}

std::string Cloud::PrepareFileToUploading(std::string const & fileName, std::string & hash)
{
  // 1. Get path to the original uploading file.
  std::string filePath;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto const it = m_files.find(fileName);
    if (it == m_files.end())
      return {};
    filePath = it->second;
  }
  if (!GetPlatform().IsFileExistsByFullPath(filePath))
    return {};

  // 2. Calculate SHA1 of the original uploading file.
  auto const originalSha1 = coding::SHA1::CalculateBase64(filePath);
  if (originalSha1.empty())
    return {};

  // 3. Create a temporary file from the original uploading file.
  auto name = ExtractFileNameWithoutExtension(filePath);
  auto const tmpPath = base::JoinFoldersToPath(GetPlatform().TmpDir(), name + ".tmp");
  if (!base::CopyFileX(filePath, tmpPath))
    return {};

  SCOPE_GUARD(tmpFileGuard, std::bind(&base::DeleteFileX, std::cref(tmpPath)));

  // 4. Calculate SHA1 of the temporary file and compare with original one.
  // Original file can be modified during copying process, so we have to
  // compare original file with the temporary file after copying.
  auto const tmpSha1 = coding::SHA1::CalculateBase64(tmpPath);
  if (originalSha1 != tmpSha1)
    return {};

  auto const outputPath = base::JoinFoldersToPath(GetPlatform().TmpDir(),
                                                name + ".uploaded");

  // 5. Convert temporary file and save to output path.
  CHECK(m_params.m_backupConverter, ());
  auto const convertionResult = m_params.m_backupConverter(tmpPath, outputPath);
  hash = convertionResult.m_hash;
  if (convertionResult.m_isSuccessful)
    return outputPath;
  
  return {};
}

Cloud::RequestResult Cloud::CreateSnapshot(std::vector<std::string> const & files) const
{
  ASSERT(!files.empty(), ());
  auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerCreateSnapshotMethod);
  return CloudRequest<SnapshotCreateRequestData>(url, GetAccessToken(), files);
}

Cloud::RequestResult Cloud::FinishSnapshot() const
{
  auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerFinishSnapshotMethod);
  return CloudRequest<SnapshotRequestData>(url, GetAccessToken());
}

Cloud::SnapshotResult Cloud::GetBestSnapshot() const
{
  auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerBestSnapshotMethod);

  SnapshotResult result;
  auto responseHandler = [&result, &url](
    int code, std::string const & serverResponse) -> Cloud::RequestResult
  {
    if (IsSuccessfulResultCode(code))
    {
      ParseRequestJsonResult(url, serverResponse, result);
      return {Cloud::RequestStatus::Ok, {}};
    }

    // Server return 404 in case of snapshot absence.
    if (code == 404)
      return {Cloud::RequestStatus::Ok, {}};

    return {Cloud::RequestStatus::NetworkError, serverResponse};
  };

  result.m_requestResult =
      CloudRequestWithResult<SnapshotRequestData>(url, GetAccessToken(), responseHandler);

  return result;
}

Cloud::UploadingResult Cloud::RequestUploading(std::string const & filePath) const
{
  auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerUploadMethod);
  return CloudRequestWithJsonResult<UploadingRequestData, UploadingResult>(
    url, GetAccessToken(), filePath);
}

Cloud::RequestResult Cloud::ExecuteUploading(UploadingResponseData const & responseData,
                                             std::string const & filePath)
{
  ASSERT(!responseData.m_url.empty(), ());
  ASSERT(!responseData.m_method.empty(), ());

  static std::string const kStatErrors[] = {"download_server", "fallback_server"};

  std::vector<std::string> urls;
  urls.push_back(responseData.m_url);
  if (!responseData.m_fallbackUrl.empty())
    urls.push_back(responseData.m_fallbackUrl);

  std::string errorStr;
  int code = 0;
  for (size_t i = 0; i < urls.size(); ++i)
  {
    platform::HttpUploader request;
    request.SetUrl(urls[i]);
    request.SetMethod(responseData.m_method);
    for (auto const & f : responseData.m_fields)
    {
      ASSERT_EQUAL(f.size(), 2, ());
      request.SetParam(f[0], f[1]);
    }
    request.SetFilePath(filePath);

    auto const result = request.Upload();
    code = result.m_httpCode;
    if (IsSuccessfulResultCode(code))
      return {RequestStatus::Ok, {}};

    errorStr = strings::to_string(code) + " " + result.m_description;
    if (code >= 500 && code < 600)
    {
      ASSERT_LESS_OR_EQUAL(i, ARRAY_SIZE(kStatErrors), ());
      alohalytics::TStringMap details{
          {"service", m_params.m_serverPathName}, {"type", kStatErrors[i]}, {"error", errorStr}};
      alohalytics::Stats::Instance().LogEvent("Cloud_Backup_error", details);
      return {RequestStatus::NetworkError, errorStr};
    }
  }

  return {code == 403 ? RequestStatus::Forbidden : RequestStatus::NetworkError, errorStr};
}

Cloud::RequestResult Cloud::NotifyAboutUploading(std::string const & filePath,
                                                 uint64_t fileSize) const
{
  auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerNotifyMethod);
  return CloudRequest<NotifyRequestData>(url, GetAccessToken(), filePath, fileSize);
}

Cloud::EntryPtr Cloud::FindOutdatedEntry() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto const & entry : m_index.m_entries)
  {
    if (entry->m_isOutdated)
      return entry;
  }
  return nullptr;
}

void Cloud::FinishUploading(SynchronizationResult result, std::string const & errorStr)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_uploadingStarted)
      return;

    if (result == SynchronizationResult::UserInterrupted)
    {
      // If the user interrupts uploading, we consider all files as up-to-dated.
      // The files will be checked and uploaded (if necessary) next time.
      m_index.m_isOutdated = false;
      for (auto & entry : m_index.m_entries)
        entry->m_isOutdated = false;
    }
    else
    {
      if (result == SynchronizationResult::Success)
      {
        m_index.m_isOutdated = false;
        m_index.m_lastSyncTimestamp = static_cast<uint64_t>(
            duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
      }
      else
      {
        m_index.m_isOutdated = true;
      }
    }
    m_uploadingStarted = false;
    SaveIndexImpl();
  }

  if (result == SynchronizationResult::AuthError)
    ThreadSafeCallback<InvalidTokenHandler>([this]() { return m_onInvalidToken; });

  ThreadSafeCallback<SynchronizationFinishedHandler>(
      [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Backup, result,
      errorStr);
}

void Cloud::SetAccessToken(std::string const & token)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken = token;
}

std::string Cloud::GetAccessToken() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_accessToken;
}

bool Cloud::IsRestoringEnabledCommonImpl(std::string & reason) const
{
  if (m_state != State::Enabled)
  {
    reason = "Cloud is not enabled";
    return false;
  }
  
  if (!m_indexUpdated)
  {
    reason = "Cloud is not initialized";
    return false;
  }
  
  if (m_accessToken.empty())
  {
    reason = "User is not authenticated";
    return false;
  }
  
  return true;
}

bool Cloud::IsRequestRestoringEnabled(std::string & reason) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!IsRestoringEnabledCommonImpl(reason))
    return false;
  
  if (m_restoringState != RestoringState::None)
  {
    reason = "Restoring process exists";
    return false;
  }
  
  return true;
}

bool Cloud::IsApplyRestoringEnabled(std::string & reason) const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!IsRestoringEnabledCommonImpl(reason))
    return false;
  
  if (m_restoringState != RestoringState::Requested)
  {
    reason = "Restoring process does not exist";
    return false;
  }
  
  if (m_bestSnapshotData.m_deviceId.empty())
  {
    reason = "Backup is absent";
    return false;
  }
  
  return true;
}

void Cloud::RequestRestoring()
{
  FinishUploading(SynchronizationResult::UserInterrupted, {});
  
  ThreadSafeCallback<SynchronizationStartedHandler>(
    [this]() { return m_onSynchronizationStarted; }, SynchronizationType::Restore);
  
  auto const status = GetPlatform().ConnectionStatus();
  if (status == Platform::EConnectionType::CONNECTION_NONE)
  {
    ThreadSafeCallback<SynchronizationFinishedHandler>(
      [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Restore,
      SynchronizationResult::InvalidCall, "No internet connection");
    return;
  }

  std::string reason;
  if (!IsRequestRestoringEnabled(reason))
  {
    ThreadSafeCallback<SynchronizationFinishedHandler>(
       [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Restore,
       SynchronizationResult::InvalidCall, reason);
    return;
  }
  
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_restoringState = RestoringState::Requested;
  }

  GetBestSnapshotTask(kTaskTimeoutInSeconds, 0 /* attemptIndex */);
}

void Cloud::ApplyRestoring()
{
  auto const status = GetPlatform().ConnectionStatus();
  if (status == Platform::EConnectionType::CONNECTION_NONE)
  {
    ThreadSafeCallback<SynchronizationFinishedHandler>(
      [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Restore,
      SynchronizationResult::InvalidCall, "No internet connection");
    return;
  }
  
  std::string reason;
  if (!IsApplyRestoringEnabled(reason))
  {
    ThreadSafeCallback<SynchronizationFinishedHandler>(
      [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Restore,
      SynchronizationResult::InvalidCall, reason);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_restoringState = RestoringState::Applying;
  }

  GetPlatform().RunTask(Platform::Thread::File, [this]()
  {
    // Create folder if it does not exist.
    auto const dirPath = GetRestoringFolder(m_params.m_serverPathName);
    if (!GetPlatform().IsFileExistsByFullPath(dirPath) && !Platform::MkDirChecked(dirPath))
    {
      FinishRestoring(SynchronizationResult::DiskError, "Unable create restoring directory");
      return;
    }

    // Get list of files to download.
    auto downloadingList = GetDownloadingList(dirPath);
    if (downloadingList.empty())
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_restoringState != RestoringState::Applying)
        return;

      // Try to complete restoring process.
      CompleteRestoring(dirPath);
    }
    else
    {
      // Start downloading.
      DownloadingTask(dirPath, false /* useFallbackUrl */, std::move(downloadingList));
    }
  });
}

void Cloud::CancelRestoring()
{
  FinishRestoring(SynchronizationResult::UserInterrupted, {});
}

void Cloud::GetBestSnapshotTask(uint32_t timeout, uint32_t attemptIndex)
{
  GetPlatform().RunDelayedTask(Platform::Thread::Network, seconds(timeout),
    [this, timeout, attemptIndex]()
  {
    bool isInvalidToken;
    {
      // Restoring state may be changed between tasks.
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_restoringState != RestoringState::Requested)
        return;

      isInvalidToken = m_accessToken.empty();
    }

    if (kServerUrl.empty())
    {
      FinishRestoring(SynchronizationResult::NetworkError, "Empty server url");
      return;
    }

    // Access token may become invalid between tasks.
    if (isInvalidToken)
    {
      FinishRestoring(SynchronizationResult::AuthError, "Access token is empty");
      return;
    }

    auto const result = GetBestSnapshot();
    if (result.m_isMalformed)
    {
      FinishRestoring(SynchronizationResult::NetworkError, "Malformed best snapshot response");
    }
    else if (result.m_requestResult.m_status == RequestStatus::Ok)
    {
      ProcessSuccessfulSnapshot(result);
    }
    else if (result.m_requestResult.m_status == RequestStatus::NetworkError)
    {
      // Retry request up to kRetryMaxAttempts times.
      if (attemptIndex + 1 == kRetryMaxAttempts)
      {
        FinishRestoring(SynchronizationResult::NetworkError, result.m_requestResult.m_error);
        return;
      }

      auto const retryTimeout = attemptIndex == 0 ? kRetryTimeoutInSeconds
                                                  : timeout * kRetryDegradationFactor;
      GetBestSnapshotTask(retryTimeout, attemptIndex + 1);
    }
    else if (result.m_requestResult.m_status == RequestStatus::Forbidden)
    {
      FinishRestoring(SynchronizationResult::AuthError, result.m_requestResult.m_error);
    }
  });
}

void Cloud::ProcessSuccessfulSnapshot(SnapshotResult const & result)
{
  // Check if the backup is empty.
  if (result.m_response.m_files.empty())
  {
    ThreadSafeCallback<RestoreRequestedHandler>([this]() { return m_onRestoreRequested; },
                                                RestoringRequestResult::NoBackup, "",
                                                result.m_response.m_datetime);
    FinishRestoring(SynchronizationResult::Success, {});
    return;
  }
  
  // Check if there is enough space to download backup.
  auto const totalSize = result.m_response.GetTotalSizeOfFiles();
  auto constexpr kSizeScalar = 10;
  if (totalSize * kSizeScalar >= GetPlatform().GetWritableStorageSpace())
  {
    ThreadSafeCallback<RestoreRequestedHandler>([this]() { return m_onRestoreRequested; },
                                                RestoringRequestResult::NotEnoughDiskSpace, "",
                                                result.m_response.m_datetime);
    FinishRestoring(SynchronizationResult::DiskError, {});
    return;
  }
  
  // Save snapshot data.
  bool isInterrupted;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bestSnapshotData = result.m_response;
    isInterrupted = (m_restoringState != RestoringState::Requested);
  }
  if (!isInterrupted)
  {
    ThreadSafeCallback<RestoreRequestedHandler>([this]() { return m_onRestoreRequested; },
                                                RestoringRequestResult::BackupExists,
                                                result.m_response.m_deviceName,
                                                result.m_response.m_datetime);
  }
}

void Cloud::FinishRestoring(Cloud::SynchronizationResult result, std::string const & errorStr)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_restoringState == RestoringState::None)
      return;

    // We cannot interrupt restoring process on the finalizing step.
    if (result == Cloud::SynchronizationResult::UserInterrupted &&
        m_restoringState == RestoringState::Finalizing)
    {
      return;
    }

    m_bestSnapshotData = {};
    m_restoringState = RestoringState::None;
  }

  if (result == SynchronizationResult::AuthError)
    ThreadSafeCallback<InvalidTokenHandler>([this]() { return m_onInvalidToken; });

  ThreadSafeCallback<SynchronizationFinishedHandler>(
      [this]() { return m_onSynchronizationFinished; }, SynchronizationType::Restore, result,
      errorStr);
}

std::list<Cloud::SnapshotFileData> Cloud::GetDownloadingList(std::string const & restoringDirPath)
{
  auto const snapshotFile = base::JoinPath(restoringDirPath, kSnapshotFile);
  auto const prevSnapshot = ReadSnapshotFile(snapshotFile);

  SnapshotResponseData currentSnapshot;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    currentSnapshot = m_bestSnapshotData;
  }

  // Save to use in the next sessions.
  try
  {
    auto jsonData = SerializeToJson(currentSnapshot);
    FileWriter w(snapshotFile);
    w.Write(jsonData.data(), jsonData.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", snapshotFile,
                   "reason:", exception.what()));
    FinishRestoring(SynchronizationResult::DiskError, "Could not save snapshot file");
    return {};
  }

  // If the snapshot from previous sessions is not valid, return files from new one.
  if (currentSnapshot.m_deviceId != prevSnapshot.m_deviceId ||
      currentSnapshot.m_datetime != prevSnapshot.m_datetime ||
      currentSnapshot.m_files.size() != prevSnapshot.m_files.size())
  {
    return std::list<Cloud::SnapshotFileData>(currentSnapshot.m_files.begin(),
                                              currentSnapshot.m_files.end());
  }

  // Check if some files were completely downloaded last time.
  std::list<Cloud::SnapshotFileData> result;
  for (auto & f : currentSnapshot.m_files)
  {
    auto const restoringFile = base::JoinPath(restoringDirPath, f.m_fileName);
    if (!GetPlatform().IsFileExistsByFullPath(restoringFile))
    {
      result.push_back(std::move(f));
      continue;
    }

    uint64_t fileSize = 0;
    if (!base::GetFileSize(restoringFile, fileSize) || fileSize != f.m_fileSize)
      result.push_back(std::move(f));
  }

  return result;
}

void Cloud::DownloadingTask(std::string const & dirPath, bool useFallbackUrl,
                            std::list<Cloud::SnapshotFileData> && files)
{
  GetPlatform().RunTask(Platform::Thread::Network,
    [this, dirPath, useFallbackUrl, files = std::move(files)]() mutable
  {
    std::string snapshotDeviceId;
    {
      // Check if the process was interrupted.
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_restoringState != RestoringState::Applying)
        return;
      snapshotDeviceId = m_bestSnapshotData.m_deviceId;
    }

    if (files.empty())
    {
      CompleteRestoring(dirPath);
      return;
    }

    auto const f = files.front();
    files.erase(files.begin());
    auto const filePath = base::JoinPath(dirPath, f.m_fileName);

    auto const url = BuildMethodUrl(m_params.m_serverPathName, kServerDownloadMethod);
    auto const result = CloudRequestWithJsonResult<DownloadingRequestData, DownloadingResult>(
        url, GetAccessToken(), snapshotDeviceId, f.m_fileName, f.m_datetime);

    if (result.m_isMalformed || result.m_response.m_url.empty())
    {
      FinishRestoring(SynchronizationResult::NetworkError, "Malformed downloading file response");
    }
    else if (result.m_requestResult.m_status == RequestStatus::Ok)
    {
      if (useFallbackUrl && result.m_response.m_fallbackUrl.empty())
      {
        FinishRestoring(SynchronizationResult::NetworkError, "Fallback url is absent");
        return;
      }

      platform::RemoteFile remoteFile(useFallbackUrl ? result.m_response.m_fallbackUrl
                                                     : result.m_response.m_url,
                                      {} /* accessToken */, false /* allowRedirection */);
      auto const downloadResult = remoteFile.Download(filePath);

      if (downloadResult.m_status == platform::RemoteFile::Status::Ok)
      {
        // Download next file.
        DownloadingTask(dirPath, false /* useFallbackUrl */, std::move(files));
      }
      else if (downloadResult.m_status == platform::RemoteFile::Status::DiskError)
      {
        FinishRestoring(SynchronizationResult::DiskError, downloadResult.m_description);
      }
      else if (downloadResult.m_status == platform::RemoteFile::Status::Forbidden)
      {
        FinishRestoring(SynchronizationResult::AuthError, downloadResult.m_description);
      }
      else
      {
        alohalytics::TStringMap details{
          {"service", m_params.m_serverPathName},
          {"type", useFallbackUrl ? "fallback_server" : "download_server"},
          {"error", downloadResult.m_description}};
        alohalytics::Stats::Instance().LogEvent("Cloud_Restore_error", details);

        if (!useFallbackUrl)
        {
          // Retry to download by means of fallback url.
          files.push_front(std::move(f));
          DownloadingTask(dirPath, true /* useFallbackUrl */, std::move(files));
        }
        else
        {
          FinishRestoring(SynchronizationResult::NetworkError, downloadResult.m_description);
        }
      }
    }
    else if (result.m_requestResult.m_status == RequestStatus::NetworkError)
    {
      FinishRestoring(SynchronizationResult::NetworkError, result.m_requestResult.m_error);
    }
    else if (result.m_requestResult.m_status == RequestStatus::Forbidden)
    {
      FinishRestoring(SynchronizationResult::AuthError, result.m_requestResult.m_error);
    }
  });
}

void Cloud::CompleteRestoring(std::string const & dirPath)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, dirPath]()
  {
    if (!m_params.m_restoreConverter)
    {
      FinishRestoring(SynchronizationResult::InvalidCall, "Restore converter is not set");
      return;
    }
    
    // Check files and convert them to expected format.
    SnapshotResponseData currentSnapshot;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      currentSnapshot = m_bestSnapshotData;
    }

    RestoredFilesCollection convertedFiles;
    convertedFiles.reserve(currentSnapshot.m_files.size());
    for (auto & f : currentSnapshot.m_files)
    {
      auto const restoringFile = base::JoinPath(dirPath, f.m_fileName);
      if (!GetPlatform().IsFileExistsByFullPath(restoringFile))
      {
        FinishRestoring(SynchronizationResult::DiskError, "Restored file is absent");
        return;
      }

      uint64_t fileSize = 0;
      if (!base::GetFileSize(restoringFile, fileSize) || fileSize != f.m_fileSize)
      {
        std::string const str = "Restored file has incorrect size. Expected size = " +
                                strings::to_string(f.m_fileSize) +
                                ". Server size =" + strings::to_string(fileSize);
        FinishRestoring(SynchronizationResult::DiskError, str);
        return;
      }

      auto const fn = f.m_fileName + ".converted";
      auto const convertedFile = base::JoinPath(dirPath, fn);
      auto const convertionResult = m_params.m_restoreConverter(restoringFile, convertedFile);
      if (!convertionResult.m_isSuccessful)
      {
        FinishRestoring(SynchronizationResult::DiskError, "Restored file conversion error");
        return;
      }
      convertedFiles.emplace_back(fn, convertionResult.m_hash);
    }

    // Check if the process was interrupted and start finalizing.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_restoringState != RestoringState::Applying)
        return;

      m_restoringState = RestoringState::Finalizing;
    }

    GetPlatform().RunTask(Platform::Thread::Gui,
                          [this, dirPath, convertedFiles = std::move(convertedFiles)]() mutable
    {
      ThreadSafeCallback<RestoredFilesPreparedHandler>([this]() { return m_onRestoredFilesPrepared; });
      ApplyRestoredFiles(dirPath, std::move(convertedFiles));
    });
  });
}

void Cloud::ApplyRestoredFiles(std::string const & dirPath, RestoredFilesCollection && files)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, dirPath, files = std::move(files)]()
  {
    // Delete all files in the destination folder.
    if (GetPlatform().IsFileExistsByFullPath(m_params.m_restoringFolder) &&
        !GetPlatform().RmDirRecursively(m_params.m_restoringFolder))
    {
      FinishRestoring(SynchronizationResult::DiskError, "Could not delete restoring folder");
      return;
    }

    // Move files.
    if (!GetPlatform().MkDirChecked(m_params.m_restoringFolder))
    {
      FinishRestoring(SynchronizationResult::DiskError, "Could not create restoring folder");
      return;
    }

    RestoredFilesCollection readyFiles;
    readyFiles.reserve(files.size());
    for (auto const & f : files)
    {
      auto const restoredFile = base::JoinPath(dirPath, f.m_filename);
      auto const finalFilename =
          base::FilenameWithoutExt(f.m_filename) + m_params.m_restoredFileExtension;
      auto const readyFile = base::JoinPath(m_params.m_restoringFolder, finalFilename);
      if (!base::RenameFileX(restoredFile, readyFile))
      {
        FinishRestoring(SynchronizationResult::DiskError, "Restored file moving error");
        return;
      }
      readyFiles.emplace_back(readyFile, f.m_hash);
    }

    // Reset upload index to the restored state.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_files.clear();
      auto const lastSyncTimestampInSec = m_bestSnapshotData.m_datetime / 1000;
      UpdateIndexByRestoredFilesImpl(readyFiles, lastSyncTimestampInSec);
    }

    // Delete temporary directory.
    GetPlatform().RmDirRecursively(dirPath);
    FinishRestoring(SynchronizationResult::Success, {});
  });
}

//static
Cloud::State Cloud::GetCloudState(std::string const & paramName)
{
  int stateValue;
  if (!settings::Get(paramName, stateValue))
  {
    stateValue = static_cast<int>(State::Unknown);
    settings::Set(paramName, stateValue);
  }
  return static_cast<State>(stateValue);
}
