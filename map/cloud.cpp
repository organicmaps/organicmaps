#include "map/cloud.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/zip_creator.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"
#include "platform/http_uploader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "3party/liboauthcpp/src/SHA1.h"
#include "3party/liboauthcpp/src/base64.h"

//#define STAGE_CLOUD_SERVER
#include "private.h"

using namespace std::chrono;

namespace
{
uint32_t constexpr kUploadTaskTimeoutInSeconds = 1;
uint64_t constexpr kUpdateTimeoutInHours = 24;
uint32_t constexpr kRetryMaxAttempts = 3;
uint32_t constexpr kRetryTimeoutInSeconds = 5;
uint32_t constexpr kRetryDegradationFactor = 2;

uint64_t constexpr kMaxWwanUploadingSizeInBytes = 10 * 1024; // 10Kb
uint64_t constexpr kMaxUploadingFileSizeInBytes = 100 * 1024 * 1024; // 100Mb

std::string const kServerUrl = CLOUD_URL;
std::string const kCloudServerVersion = "v1";
std::string const kCloudServerUploadMethod = "upload_url";
std::string const kCloudServerNotifyMethod = "upload_succeeded";
std::string const kApplicationJson = "application/json";

std::string GetIndexFilePath(std::string const & indexName)
{
  return my::JoinPath(GetPlatform().SettingsDir(), indexName);
}

std::string BuildUploadingUrl(std::string const & serverPathName)
{
  if (kServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kServerUrl << "/"
     << kCloudServerVersion << "/"
     << serverPathName << "/"
     << kCloudServerUploadMethod << "/";
  return ss.str();
}

std::string BuildNotificationUrl(std::string const & serverPathName)
{
  if (kServerUrl.empty())
    return {};

  std::ostringstream ss;
  ss << kServerUrl << "/"
     << kCloudServerVersion << "/"
     << serverPathName << "/"
     << kCloudServerNotifyMethod << "/";
  return ss.str();
}

std::string BuildAuthenticationToken(std::string const & accessToken)
{
  return "Bearer " + accessToken;
}

std::string ExtractFileName(std::string const & filePath)
{
  std::string path = filePath;
  my::GetNameFromFullPath(path);
  return path;
}

std::string CalculateSHA1(std::string const & filePath)
{
  uint32_t constexpr kFileBufferSize = 8192;
  try
  {
    my::FileData file(filePath, my::FileData::OP_READ);
    uint64_t const fileSize = file.Size();

    CSHA1 sha1;
    uint64_t currSize = 0;
    unsigned char buffer[kFileBufferSize];
    while (currSize < fileSize)
    {
      auto const toRead = std::min(kFileBufferSize, static_cast<uint32_t>(fileSize - currSize));
      file.Read(currSize, buffer, toRead);
      sha1.Update(buffer, toRead);
      currSize += toRead;
    }
    sha1.Final();
    return base64_encode(sha1.m_digest, ARRAY_SIZE(sha1.m_digest));
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LERROR, ("Error reading file:", filePath, ex.Msg()));
  }
  return {};
}

std::string BuildUploadingRequestDataJson(std::string const & filePath)
{
  std::string jsonBody;
  Cloud::UploadingRequestData data;
  data.m_alohaId = GetPlatform().UniqueClientId();
  data.m_deviceName = GetPlatform().DeviceName();
  data.m_fileName = ExtractFileName(filePath);

  using Sink = MemWriter<string>;
  Sink sink(jsonBody);
  coding::SerializerJson<Sink> serializer(sink);
  serializer(data);
  return jsonBody;
}

bool IsSuccessfulResultCode(int resultCode)
{
  return resultCode >= 200 && resultCode < 300;
}
}  // namespace

Cloud::Cloud(CloudParams && params)
  : m_params(std::move(params))
{
  ASSERT(!m_params.m_indexName.empty(), ());
  ASSERT(!m_params.m_serverPathName.empty(), ());
  ASSERT(!m_params.m_settingsParamName.empty(), ());
  ASSERT(!m_params.m_zipExtension.empty(), ());

  int stateValue;
  if (!settings::Get(m_params.m_settingsParamName, stateValue))
  {
    stateValue = static_cast<int>(State::Unknown);
    settings::Set(m_params.m_settingsParamName, stateValue);
  }

  m_state = static_cast<State>(stateValue);
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
                                       SynchronizationFinishedHandler && onSynchronizationFinished)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_onSynchronizationStarted = std::move(onSynchronizationStarted);
  m_onSynchronizationFinished = std::move(onSynchronizationFinished);
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
    my::DeleteFileX(m_params.m_indexName);
    m_files.clear();
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
    m_files[ExtractFileName(filePath)] = filePath;

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
  std::string data;
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(data);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", indexFilePath,
      "reason:", exception.what()));
    return false;
  }

  // Parse index file.
  if (data.empty())
    return false;

  try
  {
    Index index;
    coding::DeserializerJson deserializer(data);
    deserializer(index);

    std::lock_guard<std::mutex> lock(m_mutex);
    std::swap(m_index, index);
  }
  catch (my::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing file:", indexFilePath,
      "reason:", exception.what()));
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
      MarkModifiedImpl(path.second);
    m_index.m_lastUpdateInHours = h;
    m_index.m_isOutdated = true;

    // Erase disappeared files from index.
    my::EraseIf(m_index.m_entries, [this](EntryPtr const & entity) {
      return m_files.find(entity->m_name) == m_files.end();
    });

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

void Cloud::SortEntriesBeforeUploadingImpl()
{
  std::sort(m_index.m_entries.begin(), m_index.m_entries.end(),
            [](EntryPtr const & lhs, EntryPtr const & rhs)
  {
    return lhs->m_sizeInBytes < rhs->m_sizeInBytes;
  });
}

void Cloud::MarkModifiedImpl(std::string const & filePath)
{
  uint64_t fileSize = 0;
  if (!my::GetFileSize(filePath, fileSize))
    return;

  // We do not work with files which size is more than kMaxUploadingFileSizeInBytes.
  if (fileSize > kMaxUploadingFileSizeInBytes)
    return;

  auto const fileName = ExtractFileName(filePath);
  auto entryPtr = GetEntryImpl(fileName);
  if (entryPtr)
  {
    entryPtr->m_isOutdated = true;
    entryPtr->m_sizeInBytes = fileSize;
  }
  else
  {
    m_index.m_entries.emplace_back(
      std::make_shared<Entry>(fileName, fileSize, true /* m_isOutdated */));
  }
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

  std::string jsonData;
  {
    using Sink = MemWriter<string>;
    Sink sink(jsonData);
    coding::SerializerJson<Sink> serializer(sink);
    serializer(m_index);
  }

  auto const indexFilePath = GetIndexFilePath(m_params.m_indexName);
  try
  {
    FileWriter w(indexFilePath);
    w.Write(jsonData.c_str(), jsonData.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LERROR, ("Exception while writing file:", indexFilePath,
                 "reason:", exception.what()));
  }
}

void Cloud::ScheduleUploading()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != State::Enabled || !m_index.m_isOutdated ||
        m_accessToken.empty() || m_uploadingStarted || !m_indexUpdated)
    {
      return;
    }

    auto const status = GetPlatform().ConnectionStatus();
    auto const totalUploadingSize = CalculateUploadingSizeImpl();
    if (status == Platform::EConnectionType::CONNECTION_NONE ||
        (status == Platform::EConnectionType::CONNECTION_WWAN &&
         totalUploadingSize > kMaxWwanUploadingSizeInBytes))
    {
      return;
    }

    SortEntriesBeforeUploadingImpl();
    m_uploadingStarted = true;
  }

  if (m_onSynchronizationStarted != nullptr)
    m_onSynchronizationStarted();

  auto entry = FindOutdatedEntry();
  if (entry != nullptr)
    ScheduleUploadingTask(entry, kUploadTaskTimeoutInSeconds, 0 /* attemptIndex */);
  else
    FinishUploading(SynchronizationResult::Success, {});
}

void Cloud::ScheduleUploadingTask(EntryPtr const & entry, uint32_t timeout,
                                  uint32_t attemptIndex)
{
  GetPlatform().RunDelayedTask(Platform::Thread::Network, seconds(timeout),
                               [this, entry, timeout, attemptIndex]()
  {
    ASSERT(m_state == State::Enabled, ());
    ASSERT(!m_accessToken.empty(), ());
    ASSERT(m_uploadingStarted, ());
    ASSERT(entry->m_isOutdated, ());

    auto const uploadingUrl = BuildUploadingUrl(m_params.m_serverPathName);
    if (uploadingUrl.empty())
    {
      FinishUploading(SynchronizationResult::NetworkError, "Empty uploading url");
      return;
    }

    // Prepare file to uploading.
    auto const uploadedName = PrepareFileToUploading(entry->m_name);
    auto deleteAfterUploading = [uploadedName]() {
      if (!uploadedName.empty())
        my::DeleteFileX(uploadedName);
    };
    MY_SCOPE_GUARD(deleteAfterUploadingGuard, deleteAfterUploading);

    if (uploadedName.empty())
    {
      FinishUploading(SynchronizationResult::DiskError, "File preparation error");
      return;
    }

    // Upload only if SHA1 is not equal to previous one.
    auto const sha1 = CalculateSHA1(uploadedName);
    if (sha1.empty())
    {
      FinishUploading(SynchronizationResult::DiskError, "SHA1 calculation error");
      return;
    }

    if (entry->m_hash != sha1)
    {
      // Request uploading.
      auto const result = RequestUploading(uploadingUrl, uploadedName);
      if (result.m_requestResult.m_status == RequestStatus::NetworkError)
      {
        // Retry uploading request up to kRetryMaxAttempts times.
        if (attemptIndex + 1 == kRetryMaxAttempts)
        {
          FinishUploading(SynchronizationResult::NetworkError, result.m_requestResult.m_error);
          return;
        }

        auto const retryTimeout = attemptIndex == 0 ? kRetryTimeoutInSeconds
                                                    : timeout * kRetryDegradationFactor;
        ScheduleUploadingTask(entry, retryTimeout, attemptIndex + 1);
        return;
      }
      else if (result.m_requestResult.m_status == RequestStatus::Forbidden)
      {
        // Finish uploading and notify about invalid access token.
        if (m_onInvalidToken != nullptr)
          m_onInvalidToken();

        FinishUploading(SynchronizationResult::AuthError, result.m_requestResult.m_error);
        return;
      }

      // Execute uploading.
      auto const executeResult = ExecuteUploading(result.m_response, uploadedName);
      if (executeResult.m_status != RequestStatus::Ok)
      {
        FinishUploading(SynchronizationResult::NetworkError, executeResult.m_error);
        return;
      }

      // Notify about successful uploading.
      auto const notificationResult =
          NotifyAboutUploading(BuildNotificationUrl(m_params.m_serverPathName), uploadedName);
      if (executeResult.m_status != RequestStatus::Ok)
      {
        FinishUploading(SynchronizationResult::NetworkError, notificationResult.m_error);
        return;
      }
    }

    // Mark entry as not outdated.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      entry->m_isOutdated = false;
      entry->m_hash = sha1;
      SaveIndexImpl();
    }

    // Schedule next uploading task.
    auto nextEntry = FindOutdatedEntry();
    if (nextEntry != nullptr)
      ScheduleUploadingTask(nextEntry, kUploadTaskTimeoutInSeconds, 0 /* attemptIndex */);
    else
      FinishUploading(SynchronizationResult::Success, {});
  });
}

std::string Cloud::PrepareFileToUploading(std::string const & fileName)
{
  // 1. Get path to the original uploading file.
  std::string filePath;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto const it = m_files.find(fileName);
    if (it == m_files.end())
      return {};
    filePath = it->second;
    if (!GetPlatform().IsFileExistsByFullPath(filePath))
      return {};
  }

  // 2. Calculate SHA1 of the original uploading file.
  auto const originalSha1 = CalculateSHA1(filePath);
  if (originalSha1.empty())
    return {};

  // 3. Create a temporary file from the original uploading file.
  auto name = ExtractFileName(filePath);
  auto const tmpPath = my::JoinFoldersToPath(GetPlatform().TmpDir(), name);
  if (!my::CopyFileX(filePath, tmpPath))
    return {};
  
  MY_SCOPE_GUARD(tmpFileGuard, std::bind(&my::DeleteFileX, std::cref(tmpPath)));

  // 4. Calculate SHA1 of the temporary file and compare with original one.
  // Original file can be modified during copying process, so we have to
  // compare original file with the temporary file after copying.
  auto const tmpSha1 = CalculateSHA1(tmpPath);
  if (originalSha1 != tmpSha1)
    return {};

  // 5. If the file is zipped return path to the temporary file.
  auto ext = my::GetFileExtension(tmpPath);
  strings::AsciiToLower(ext);
  if (ext == m_params.m_zipExtension)
  {
    tmpFileGuard.release(); // Do not delete temporary file here.
    return tmpPath;
  }

  // 6. Zip file and return path.
  my::GetNameWithoutExt(name);
  auto const zipPath = my::JoinFoldersToPath(GetPlatform().TmpDir(),
                                             name + m_params.m_zipExtension);
  if (CreateZipFromPathDeflatedAndDefaultCompression(tmpPath, zipPath))
    return zipPath;

  return {};
}

Cloud::UploadingResult Cloud::RequestUploading(std::string const & uploadingUrl,
                                               std::string const & filePath) const
{
  UploadingResult result;

  platform::HttpClient request(uploadingUrl);
  request.SetRawHeader("Accept", kApplicationJson);
  request.SetRawHeader("Authorization", BuildAuthenticationToken(m_accessToken));
  request.SetBodyData(BuildUploadingRequestDataJson(filePath), kApplicationJson);

  if (request.RunHttpRequest() && !request.WasRedirected())
  {
    int const resultCode = request.ErrorCode();
    if (IsSuccessfulResultCode(resultCode))
    {
      result.m_requestResult = {RequestStatus::Ok, {}};
      coding::DeserializerJson des(request.ServerResponse());
      des(result.m_response);
      return result;
    }

    if (resultCode == 403)
    {
      LOG(LWARNING, ("Access denied for", uploadingUrl));
      result.m_requestResult = {RequestStatus::Forbidden, request.ServerResponse()};
      return result;
    }
  }

  result.m_requestResult = {RequestStatus::NetworkError, request.ServerResponse()};
  return result;
}

Cloud::RequestResult Cloud::ExecuteUploading(UploadingResponseData const & responseData,
                                             std::string const & filePath)
{
  ASSERT(!responseData.m_url.empty(), ());
  ASSERT(!responseData.m_method.empty(), ());

  platform::HttpUploader request;
  request.SetUrl(responseData.m_url);
  request.SetMethod(responseData.m_method);
  std::map<std::string, std::string> params;
  for (auto const & f : responseData.m_fields)
  {
    ASSERT_EQUAL(f.size(), 2, ());
    params.insert(std::make_pair(f[0], f[1]));
  }
  request.SetParams(params);
  request.SetFilePath(filePath);

  auto const result = request.Upload();
  if (IsSuccessfulResultCode(result.m_httpCode))
    return {RequestStatus::Ok, {}};

  auto const errorStr = strings::to_string(result.m_httpCode) + " " + result.m_description;
  if (result.m_httpCode == 403)
    return {RequestStatus::Forbidden, errorStr};

  return {RequestStatus::NetworkError, errorStr};
}

Cloud::RequestResult Cloud::NotifyAboutUploading(std::string const & notificationUrl,
                                                 std::string const & filePath) const
{
  platform::HttpClient request(notificationUrl);
  request.SetRawHeader("Accept", kApplicationJson);
  request.SetRawHeader("Authorization", BuildAuthenticationToken(m_accessToken));
  request.SetBodyData(BuildUploadingRequestDataJson(filePath), kApplicationJson);

  if (request.RunHttpRequest() && !request.WasRedirected())
  {
    int const resultCode = request.ErrorCode();
    if (IsSuccessfulResultCode(resultCode))
      return {RequestStatus::Ok, {}};

    if (resultCode == 403)
    {
      LOG(LWARNING, ("Access denied for", notificationUrl));
      return {RequestStatus::Forbidden, request.ServerResponse()};
    }
  }

  return {RequestStatus::NetworkError, request.ServerResponse()};
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
    m_index.m_isOutdated = (result != SynchronizationResult::Success);
    if (result == SynchronizationResult::Success)
    {
      m_index.m_lastSyncTimestamp = static_cast<uint64_t>(
        duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
    }
    m_uploadingStarted = false;
    SaveIndexImpl();
  }

  if (m_onSynchronizationFinished != nullptr)
    m_onSynchronizationFinished(result, errorStr);
}

void Cloud::SetAccessToken(std::string const & token)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accessToken = token;
}
