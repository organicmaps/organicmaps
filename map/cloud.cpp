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

#include <algorithm>
#include <chrono>
#include <sstream>

#define STAGE_CLOUD_SERVER
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

std::string const kServerUrl = CLOUD_URL;
std::string const kCloudServerVersion = "v1";
std::string const kCloudServerUploadMethod = "upload_url";

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

std::string BuildAuthenticationToken(std::string const & accessToken)
{
  return "Bearer " + accessToken;
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
  m_files.insert(filePaths.cbegin(), filePaths.end());

  if (m_state != State::Enabled)
    return;

  GetPlatform().RunTask(Platform::Thread::File, [this]() { LoadIndex(); });
}

void Cloud::MarkModified(std::string const & filePath)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_state != State::Enabled)
    return;

  m_files.insert(filePath);
  MarkModifiedImpl(filePath, false /* checkSize */);
}

uint64_t Cloud::GetLastSynchronizationTimestamp() const
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
  ReadIndex();
  UpdateIndex();
  ScheduleUploading();
}

void Cloud::ReadIndex()
{
  auto const indexFilePath = GetIndexFilePath(m_params.m_indexName);
  if (!GetPlatform().IsFileExistsByFullPath(indexFilePath))
    return;

  // Read index file.
  std::string data;
  try
  {
    FileReader r(indexFilePath);
    r.ReadAsString(data);
  }
  catch (FileReader::Exception const & exception)
  {
    data.clear();
    LOG(LWARNING, ("Exception while reading file:", indexFilePath,
      "reason:", exception.what()));
  }

  // Parse index file.
  if (!data.empty())
  {
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
  }
}

void Cloud::UpdateIndex()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Now we process files ONLY if update time is out.
  auto const h = static_cast<uint64_t>(
    duration_cast<hours>(system_clock::now().time_since_epoch()).count());
  if (h >= m_index.m_lastUpdateInHours + kUpdateTimeoutInHours)
  {
    for (auto const & path : m_files)
      MarkModifiedImpl(path, true /* checkSize */);
    m_index.m_lastUpdateInHours = h;
    m_index.m_isOutdated = true;

    SaveIndexImpl();
  }
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

void Cloud::MarkModifiedImpl(std::string const & filePath, bool checkSize)
{
  uint64_t fileSize = 0;
  if (!my::GetFileSize(filePath, fileSize))
    return;

  auto entryPtr = GetEntryImpl(filePath);
  if (entryPtr)
  {
    entryPtr->m_isOutdated = checkSize ? (entryPtr->m_sizeInBytes != fileSize) : true;
    entryPtr->m_sizeInBytes = fileSize;
  }
  else
  {
    m_index.m_entries.emplace_back(
      std::make_shared<Entry>(filePath, fileSize, true /* m_isOutdated */));
  }
}

Cloud::EntryPtr Cloud::GetEntryImpl(std::string const & filePath) const
{
  auto it = std::find_if(m_index.m_entries.begin(), m_index.m_entries.end(),
                         [filePath](EntryPtr ptr) { return ptr->m_name == filePath; });
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
    if (m_state != State::Enabled || !m_index.m_isOutdated || m_accessToken.empty() ||
        m_uploadingStarted)
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
    bool needDeleteFileAfterUploading = false;
    auto const uploadedName = PrepareFileToUploading(entry->m_name, needDeleteFileAfterUploading);
    auto deleteAfterUploading = [needDeleteFileAfterUploading, uploadedName]() {
      if (needDeleteFileAfterUploading)
        my::DeleteFileX(uploadedName);
    };
    MY_SCOPE_GUARD(deleteAfterUploadingGuard, deleteAfterUploading);

    if (uploadedName.empty())
    {
      FinishUploading(SynchronizationResult::DiskError, {});
      return;
    }

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
      // Finish uploading and nofity about invalid access token.
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

    // Mark entry as not outdated.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      entry->m_isOutdated = false;
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

std::string Cloud::PrepareFileToUploading(std::string const & filePath,
                                          bool & needDeleteAfterUploading)
{
  needDeleteAfterUploading = false;
  auto ext = my::GetFileExtension(filePath);
  strings::AsciiToLower(ext);
  if (ext == m_params.m_zipExtension)
    return filePath;

  std::string name = filePath;
  my::GetNameFromFullPath(name);
  my::GetNameWithoutExt(name);
  auto const zipPath = my::JoinFoldersToPath(GetPlatform().TmpDir(), name + m_params.m_zipExtension);

  if (CreateZipFromPathDeflatedAndDefaultCompression(filePath, zipPath))
  {
    needDeleteAfterUploading = true;
    return zipPath;
  }
  return {};
}

Cloud::UploadingResult Cloud::RequestUploading(std::string const & uploadingUrl,
                                               std::string const & filePath) const
{
  static std::string const kApplicationJson = "application/json";

  UploadingResult result;

  platform::HttpClient request(uploadingUrl);
  request.SetRawHeader("Accept", kApplicationJson);
  request.SetRawHeader("Authorization", BuildAuthenticationToken(m_accessToken));

  std::string jsonBody;
  {
    UploadingRequestData data;
    data.m_alohaId = GetPlatform().UniqueClientId();
    data.m_deviceName = GetPlatform().DeviceName();
    data.m_fileName = filePath;
    my::GetNameFromFullPath(data.m_fileName);

    using Sink = MemWriter<string>;
    Sink sink(jsonBody);
    coding::SerializerJson<Sink> serializer(sink);
    serializer(data);
  }
  request.SetBodyData(std::move(jsonBody), kApplicationJson);

  if (request.RunHttpRequest() && !request.WasRedirected())
  {
    int const resultCode = request.ErrorCode();
    bool const isSuccessfulCode = (resultCode == 200 || resultCode == 201);
    if (isSuccessfulCode)
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
  if (result.m_httpCode == 200 || result.m_httpCode == 201)
    return {RequestStatus::Ok, {}};

  auto const errorStr = strings::to_string(result.m_httpCode) + " " + result.m_description;
  if (result.m_httpCode == 403)
    return {RequestStatus::Forbidden, errorStr};

  return {RequestStatus::NetworkError, errorStr};
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
