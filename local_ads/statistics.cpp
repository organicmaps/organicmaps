#include "local_ads/statistics.hpp"
#include "local_ads/config.hpp"
#include "local_ads/file_helpers.hpp"

#include "platform/http_client.hpp"
#include "platform/network_policy.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/point_coding.hpp"
#include "coding/sha1.hpp"
#include "coding/url_encode.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/zlib.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <functional>
#include <memory>
#include <sstream>
#include <utility>

#include "private.h"

namespace
{
std::string const kStatisticsFolderName = "local_ads_stats";
std::string const kStatisticsExt = ".dat";

uint64_t constexpr kMaxFilesSizeInBytes = 10 * 1024 * 1024;
float const kEventsDisposingRate = 0.2f;

auto constexpr kSendingTimeout = std::chrono::hours(1);
int64_t constexpr kEventMaxLifetimeInSeconds = 24 * 183 * 3600;  // About half of year.
auto constexpr kDeletionPeriod = std::chrono::hours(24);

std::string const kStatisticsServer = LOCAL_ADS_STATISTICS_SERVER_URL;

void WriteMetadata(FileWriter & writer, std::string const & countryId, int64_t mwmVersion,
                   local_ads::Timestamp const & ts)
{
  local_ads::WriteCountryName(writer, countryId);
  local_ads::WriteZigZag(writer, mwmVersion);
  local_ads::WriteTimestamp<std::chrono::seconds>(writer, ts);
}

void ReadMetadata(ReaderSource<FileReader> & src, std::string & countryId, int64_t & mwmVersion,
                  local_ads::Timestamp & ts)
{
  countryId = local_ads::ReadCountryName(src);
  mwmVersion = local_ads::ReadZigZag(src);
  ts = local_ads::ReadTimestamp<std::chrono::seconds>(src);
}

void WritePackedData(FileWriter & writer, local_ads::Statistics::PackedData && packedData)
{
  WriteToSink(writer, packedData.m_eventType);
  WriteToSink(writer, packedData.m_zoomLevel);
  WriteToSink(writer, packedData.m_featureIndex);
  WriteToSink(writer, packedData.m_seconds);
  local_ads::WriteZigZag(writer, packedData.m_mercator);
  WriteToSink(writer, packedData.m_accuracy);
}

template <typename ToDo>
void ReadPackedData(ReaderSource<FileReader> & src, ToDo && toDo)
{
  using PackedData = local_ads::Statistics::PackedData;

  std::string countryId;
  int64_t mwmVersion;
  local_ads::Timestamp baseTimestamp;
  ReadMetadata(src, countryId, mwmVersion, baseTimestamp);
  while (src.Size() > 0)
  {
    PackedData data;
    data.m_eventType = ReadPrimitiveFromSource<uint8_t>(src);
    data.m_zoomLevel = ReadPrimitiveFromSource<uint8_t>(src);
    data.m_featureIndex = ReadPrimitiveFromSource<uint32_t>(src);
    data.m_seconds = ReadPrimitiveFromSource<uint32_t>(src);
    data.m_mercator = local_ads::ReadZigZag(src);
    data.m_accuracy = ReadPrimitiveFromSource<uint16_t>(src);
    toDo(std::move(data), countryId, mwmVersion, baseTimestamp);
  }
}

template <typename ToDo>
void FilterEvents(std::list<local_ads::Event> const & events, std::string const & countryId,
                  int64_t mwmVersion, ToDo && toDo)
{
  for (auto const & event : events)
  {
    if (event.m_countryId != countryId || event.m_mwmVersion != mwmVersion)
      continue;
    toDo(event);
  }
}

local_ads::Timestamp GetMinTimestamp(std::list<local_ads::Event> const & events,
                                     std::string const & countryId, int64_t mwmVersion)
{
  local_ads::Timestamp minTimestamp = local_ads::Timestamp::max();
  FilterEvents(events, countryId, mwmVersion, [&minTimestamp](local_ads::Event const & event)
  {
    if (event.m_timestamp < minTimestamp)
      minTimestamp = event.m_timestamp;
  });
  return minTimestamp;
}

local_ads::Timestamp GetMaxTimestamp(std::list<local_ads::Event> const & events,
                                     std::string const & countryId, int64_t mwmVersion)
{
  local_ads::Timestamp maxTimestamp = local_ads::Timestamp::min();
  FilterEvents(events, countryId, mwmVersion, [&maxTimestamp](local_ads::Event const & event)
  {
    if (event.m_timestamp > maxTimestamp)
      maxTimestamp = event.m_timestamp;
  });
  return maxTimestamp;
}

std::string GetClientIdHash()
{
  return coding::SHA1::CalculateBase64ForString(GetPlatform().UniqueClientId());
}

std::string GetPath(std::string const & fileName)
{
  return base::JoinFoldersToPath({GetPlatform().SettingsDir(), kStatisticsFolderName}, fileName);
}

std::string GetPath(local_ads::Event const & event)
{
  return GetPath(event.m_countryId + "_" + strings::to_string(event.m_mwmVersion) + kStatisticsExt);
}

std::string StatisticsFolder()
{
  return GetPath("");
}

void CreateDirIfNotExist()
{
  std::string const statsFolder = StatisticsFolder();
  if (!GetPlatform().IsFileExistsByFullPath(statsFolder) && !Platform::MkDirChecked(statsFolder))
    MYTHROW(FileSystemException, ("Unable to find or create directory", statsFolder));
}

std::list<local_ads::Event> ReadEvents(std::string const & fileName)
{
  std::list<local_ads::Event> result;
  if (!GetPlatform().IsFileExistsByFullPath(fileName))
    return result;
  
  try
  {
    FileReader reader(fileName);
    ReaderSource<FileReader> src(reader);
    ReadPackedData(src, [&result](local_ads::Statistics::PackedData && data,
                                  std::string const & countryId, int64_t mwmVersion,
                                  local_ads::Timestamp const & baseTimestamp) {
      auto const mercatorPt = Int64ToPointObsolete(data.m_mercator, kPointCoordBits);
      result.emplace_back(static_cast<local_ads::EventType>(data.m_eventType), mwmVersion, countryId,
                          data.m_featureIndex, data.m_zoomLevel,
                          baseTimestamp + std::chrono::seconds(data.m_seconds),
                          MercatorBounds::YToLat(mercatorPt.y),
                          MercatorBounds::XToLon(mercatorPt.x), data.m_accuracy);
    });
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", fileName, ex.Msg()));
  }
  return result;
}

std::string MakeRemoteURL(std::string const & userId, std::string const & name, int64_t version)
{
  if (kStatisticsServer.empty())
    return {};

  std::ostringstream ss;
  ss << kStatisticsServer << "/";
  ss << UrlEncode(userId) << "/";
  ss << version << "/";
  ss << UrlEncode(name);
  return ss.str();
}
  
std::vector<uint8_t> SerializeForServer(std::list<local_ads::Event> const & events,
                                        std::string const & userId)
{
  using namespace std::chrono;
  ASSERT(!events.empty(), ());
  auto root = base::NewJSONObject();
  ToJSONObject(*root, "userId", userId);
  ToJSONObject(*root, "countryId", events.front().m_countryId);
  ToJSONObject(*root, "mwmVersion", events.front().m_mwmVersion);
  auto eventsNode = base::NewJSONArray();
  for (auto const & event : events)
  {
    auto eventNode = base::NewJSONObject();
    auto s = duration_cast<seconds>(event.m_timestamp.time_since_epoch()).count();
    ToJSONObject(*eventNode, "type", static_cast<uint8_t>(event.m_type));
    ToJSONObject(*eventNode, "timestamp", static_cast<int64_t>(s));
    ToJSONObject(*eventNode, "featureId", static_cast<int32_t>(event.m_featureId));
    ToJSONObject(*eventNode, "zoomLevel", event.m_zoomLevel);
    ToJSONObject(*eventNode, "latitude", event.m_latitude);
    ToJSONObject(*eventNode, "longitude", event.m_longitude);
    ToJSONObject(*eventNode, "accuracyInMeters", event.m_accuracyInMeters);
    json_array_append_new(eventsNode.get(), eventNode.release());
  }
  json_object_set_new(root.get(), "events", eventsNode.release());
  std::unique_ptr<char, JSONFreeDeleter> buffer(
    json_dumps(root.get(), JSON_COMPACT | JSON_ENSURE_ASCII));
  std::vector<uint8_t> result;
  
  using Deflate = coding::ZLib::Deflate;
  Deflate deflate(Deflate::Format::ZLib, Deflate::Level::BestCompression);
  deflate(buffer.get(), strlen(buffer.get()), std::back_inserter(result));
  return result;
}
  
bool CanUpload()
{
  auto const connectionStatus = GetPlatform().ConnectionStatus();
  if (connectionStatus == Platform::EConnectionType::CONNECTION_WIFI)
    return true;
  
  return connectionStatus == Platform::EConnectionType::CONNECTION_WWAN &&
         platform::GetCurrentNetworkPolicy().CanUse();
}
}  // namespace

namespace local_ads
{
Statistics::Statistics()
  : m_userId(GetClientIdHash())
{}
  
void Statistics::Startup()
{
  m_isEnabled = true;
  GetPlatform().RunTask(Platform::Thread::File, [this]
  {
    IndexMetadata();
    SendToServer();
  });
}

void Statistics::RegisterEvent(Event && event)
{
  if (!m_isEnabled)
    return;

  RegisterEvents({std::move(event)});
}

void Statistics::RegisterEvents(std::list<Event> && events)
{
  if (!m_isEnabled)
    return;
  GetPlatform().RunTask(Platform::Thread::File,
                        std::bind(&Statistics::ProcessEvents, this, std::move(events)));
}

void Statistics::RegisterEventSync(Event && event)
{
  std::list<Event> events = {std::move(event)};
  ProcessEvents(events);
}

void Statistics::SetEnabled(bool isEnabled)
{
  m_isEnabled = isEnabled;
}

std::list<Event> Statistics::WriteEvents(std::list<Event> & events, std::string & fileNameToRebuild)
{
  try
  {
    CreateDirIfNotExist();
    if (m_metadataCache.empty())
      IndexMetadata();

    std::unique_ptr<FileWriter> writer;

    events.sort();

    auto eventIt = events.begin();
    for (; eventIt != events.end(); ++eventIt)
    {
      Event const & event = *eventIt;
      MetadataKey const key = std::make_pair(event.m_countryId, event.m_mwmVersion);
      auto it = m_metadataCache.find(key);

      // Get metadata.
      Metadata metadata;
      bool needWriteMetadata = false;
      if (it == m_metadataCache.end())
      {
        metadata.m_timestamp = GetMinTimestamp(events, event.m_countryId, event.m_mwmVersion);
        metadata.m_fileName = GetPath(event);
        m_metadataCache[key] = metadata;
        needWriteMetadata = true;
      }
      else
      {
        metadata = it->second;
      }

      if (writer == nullptr || writer->GetName() != metadata.m_fileName)
        writer = std::make_unique<FileWriter>(metadata.m_fileName, FileWriter::OP_APPEND);

      if (needWriteMetadata)
        WriteMetadata(*writer, event.m_countryId, event.m_mwmVersion, metadata.m_timestamp);

      // Check if timestamp is out of date. In this case we have to rebuild events package.
      using namespace std::chrono;
      int64_t const s = duration_cast<seconds>(event.m_timestamp - metadata.m_timestamp).count();
      if (s < 0 || s > kEventMaxLifetimeInSeconds)
      {
        fileNameToRebuild = writer->GetName();

        // Return unprocessed events.
        std::list<Event> unprocessedEvents;
        unprocessedEvents.splice(unprocessedEvents.end(), events, eventIt, events.end());
        return unprocessedEvents;
      }

      PackedData data;
      data.m_featureIndex = event.m_featureId;
      data.m_seconds = static_cast<uint32_t>(s);
      data.m_zoomLevel = event.m_zoomLevel;
      data.m_eventType = static_cast<uint8_t>(event.m_type);
      auto const mercatorPt = MercatorBounds::FromLatLon(event.m_latitude, event.m_longitude);
      data.m_mercator = PointToInt64Obsolete(mercatorPt, kPointCoordBits);
      data.m_accuracy = event.m_accuracyInMeters;
      WritePackedData(*writer, std::move(data));
    }
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, (ex.Msg()));
  }
  return std::list<Event>();
}

void Statistics::ProcessEvents(std::list<Event> & events)
{
  bool needRebuild;
  do
  {
    std::string fileNameToRebuild;
    auto unprocessedEvents = WriteEvents(events, fileNameToRebuild);
    needRebuild = !unprocessedEvents.empty();
    if (!needRebuild)
      break;

    // The first event in the list is cause of writing interruption.
    Event event = unprocessedEvents.front();

    // Read events and merge with unprocessed ones.
    std::list<Event> newEvents = ReadEvents(fileNameToRebuild);
    newEvents.splice(newEvents.end(), std::move(unprocessedEvents));
    newEvents.sort();

    // Clip obsolete events.
    auto constexpr kLifetime = std::chrono::seconds(kEventMaxLifetimeInSeconds);
    auto const maxTimestamp = GetMaxTimestamp(newEvents, event.m_countryId, event.m_mwmVersion);
    auto newMinTimestamp = maxTimestamp - kLifetime + kDeletionPeriod;
    for (auto eventIt = newEvents.begin(); eventIt != newEvents.end();)
    {
      if (eventIt->m_countryId == event.m_countryId &&
          eventIt->m_mwmVersion == event.m_mwmVersion && eventIt->m_timestamp < newMinTimestamp)
      {
        eventIt = newEvents.erase(eventIt);
      }
      else
      {
        ++eventIt;
      }
    }

    // Update run-time cache and delete rebuilding file.
    m_metadataCache.erase(MetadataKey(event.m_countryId, event.m_mwmVersion));
    FileWriter::DeleteFileX(fileNameToRebuild);
    std::swap(events, newEvents);
  } while (needRebuild);
}

void Statistics::SendToServer()
{
  if (!m_isEnabled)
    return;

  if (CanUpload())
  {
    for (auto it = m_metadataCache.begin(); it != m_metadataCache.end(); ++it)
    {
      auto metadataKey = it->first;
      auto metadata = it->second;
      GetPlatform().RunTask(Platform::Thread::Network, [this, metadataKey = std::move(metadataKey),
                                                        metadata = std::move(metadata)]() mutable
      {
        SendFileWithMetadata(std::move(metadataKey), std::move(metadata));
      });
    }
  }
  
  // Send every |kSendingTimeout|.
  GetPlatform().RunDelayedTask(Platform::Thread::File, kSendingTimeout, [this]
  {
    SendToServer();
  });
}
  
void Statistics::SendFileWithMetadata(MetadataKey && metadataKey, Metadata && metadata)
{
  std::string const url = MakeRemoteURL(m_userId, metadataKey.first, metadataKey.second);
  if (url.empty())
    return;
  
  std::list<Event> events = ReadEvents(metadata.m_fileName);
  if (events.empty())
    return;
  
  std::string contentType = "application/octet-stream";
  std::string contentEncoding = "";
  std::vector<uint8_t> bytes = SerializeForServer(events, m_userId);
  ASSERT(!bytes.empty(), ());
  
  platform::HttpClient request(url);
  request.SetTimeout(5);    // timeout in seconds
#ifdef DEV_LOCAL_ADS_SERVER
  request.LoadHeaders(true);
  request.SetRawHeader("Host", "localads-statistics.maps.me");
#endif
  request.SetBodyData(std::string(bytes.begin(), bytes.end()), contentType, "POST",
                      contentEncoding);
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
  if (request.RunHttpRequest() && request.ErrorCode() == 200)
  {
    GetPlatform().RunTask(Platform::Thread::File, [this, metadataKey = std::move(metadataKey),
                                                   metadata = std::move(metadata)]
    {
      FileWriter::DeleteFileX(metadata.m_fileName);
      m_metadataCache.erase(metadataKey);
    });
  }
  else
  {
    LOG(LWARNING, ("Sending statistics failed:", "URL:", url, "Error code:", request.ErrorCode(),
                   metadataKey.first, metadataKey.second));
  }
}

std::list<Event> Statistics::WriteEventsForTesting(std::list<Event> const & events,
                                                   std::string & fileNameToRebuild)
{
  std::list<Event> mutableEvents = events;
  return WriteEvents(mutableEvents, fileNameToRebuild);
}

void Statistics::IndexMetadata()
{
  std::vector<std::string> files;
  GetPlatform().GetFilesByExt(StatisticsFolder(), kStatisticsExt, files);
  for (auto const & filename : files)
    ExtractMetadata(GetPath(filename));
  BalanceMemory();
}

void Statistics::ExtractMetadata(std::string const & fileName)
{
  ASSERT(GetPlatform().IsFileExistsByFullPath(fileName), ());
  try
  {
    std::string countryId;
    int64_t mwmVersion;
    Timestamp baseTimestamp;
    {
      FileReader reader(fileName);
      ReaderSource<FileReader> src(reader);
      ReadMetadata(src, countryId, mwmVersion, baseTimestamp);
    }
    MetadataKey const key = std::make_pair(countryId, mwmVersion);
    auto it = m_metadataCache.find(key);
    if (it != m_metadataCache.end())
    {
      // The only statistics file for countryId + mwmVersion must exist.
      if (it->second.m_timestamp < baseTimestamp)
        FileWriter::DeleteFileX(it->second.m_fileName);
      else
        FileWriter::DeleteFileX(fileName);
    }
    m_metadataCache[key] = Metadata(fileName, baseTimestamp);
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LWARNING, ("Error reading file:", fileName, ex.Msg()));
  }
}

void Statistics::BalanceMemory()
{
  std::map<MetadataKey, uint64_t> sizeInBytes;
  uint64_t totalSize = 0;
  for (auto const & metadata : m_metadataCache)
  {
    FileReader reader(metadata.second.m_fileName);
    sizeInBytes[metadata.first] = reader.Size();
    totalSize += reader.Size();
  }

  if (totalSize < kMaxFilesSizeInBytes)
    return;

  auto constexpr kPackedDataSize =
      sizeof(PackedData::m_featureIndex) + sizeof(PackedData::m_seconds) +
      sizeof(PackedData::m_accuracy) + sizeof(PackedData::m_mercator) +
      sizeof(PackedData::m_zoomLevel) + sizeof(PackedData::m_eventType);
  for (auto const & metadata : sizeInBytes)
  {
    auto const disposingSize = static_cast<uint64_t>(metadata.second * kEventsDisposingRate);
    auto const disposingCount = disposingSize / kPackedDataSize;

    std::string fileName = m_metadataCache[metadata.first].m_fileName;
    std::list<Event> events = ReadEvents(fileName);
    m_metadataCache.erase(metadata.first);
    FileWriter::DeleteFileX(fileName);
    if (events.size() <= disposingCount)
      continue;

    events.sort();
    auto it = events.begin();
    std::advance(it, static_cast<size_t>(disposingCount));
    events.erase(events.begin(), it);

    std::string fileNameToRebuild;
    WriteEvents(events, fileNameToRebuild);
    ASSERT(fileNameToRebuild.empty(), ());
  }
}

std::list<Event> Statistics::ReadEventsForTesting(std::string const & fileName)
{
  return ReadEvents(GetPath(fileName));
}

void Statistics::ProcessEventsForTesting(std::list<Event> const & events)
{
  std::list<Event> mutableEvents = events;
  ProcessEvents(mutableEvents);
}

void Statistics::CleanupAfterTesting()
{
  std::string const statsFolder = StatisticsFolder();
  if (GetPlatform().IsFileExistsByFullPath(statsFolder))
    GetPlatform().RmDirRecursively(statsFolder);
}
}  // namespace local_ads
