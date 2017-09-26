#include "local_ads/statistics.hpp"
#include "local_ads/config.hpp"
#include "local_ads/file_helpers.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"
#include "coding/point_to_integer.hpp"
#include "coding/url_encode.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <functional>
#include <sstream>

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

std::string GetPath(std::string const & fileName)
{
  return my::JoinFoldersToPath({GetPlatform().WritableDir(), kStatisticsFolderName}, fileName);
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
}  // namespace

namespace local_ads
{
Statistics::~Statistics()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  ASSERT(!m_isRunning, ());
}

void Statistics::Startup()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRunning)
      return;
    m_isRunning = true;
  }
  m_thread = threads::SimpleThread(&Statistics::ThreadRoutine, this);
}

void Statistics::Teardown()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isRunning)
      return;
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

bool Statistics::RequestEvents(std::list<Event> & events, bool & needToSend)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  bool const isTimeout = !m_condition.wait_for(lock, kSendingTimeout, [this]
  {
    return !m_isRunning || !m_events.empty();
  });

  if (!m_isRunning)
    return false;

  using namespace std::chrono;
  needToSend = m_isFirstSending || isTimeout ||
    (std::chrono::steady_clock::now() > (m_lastSending + kSendingTimeout));

  events = std::move(m_events);
  m_events.clear();
  return true;
}

void Statistics::RegisterEvent(Event && event)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isRunning)
    return;
  m_events.push_back(std::move(event));
  m_condition.notify_one();
}

void Statistics::RegisterEvents(std::list<Event> && events)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isRunning)
    return;
  m_events.splice(m_events.end(), std::move(events));
  m_condition.notify_one();
}

void Statistics::ThreadRoutine()
{
  std::list<Event> events;
  bool needToSend = false;
  while (RequestEvents(events, needToSend))
  {
    ProcessEvents(events);
    events.clear();

    // Send statistics to server.
    if (needToSend)
      SendToServer();
  }
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
        writer = my::make_unique<FileWriter>(metadata.m_fileName, FileWriter::OP_APPEND);

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
      data.m_mercator = PointToInt64(mercatorPt, POINT_COORD_BITS);
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

std::list<Event> Statistics::ReadEvents(std::string const & fileName) const
{
  std::list<Event> result;
  if (!GetPlatform().IsFileExistsByFullPath(fileName))
    return result;

  try
  {
    FileReader reader(fileName);
    ReaderSource<FileReader> src(reader);
    ReadPackedData(src, [&result](PackedData && data, std::string const & countryId,
                                  int64_t mwmVersion, Timestamp const & baseTimestamp) {
      auto const mercatorPt = Int64ToPoint(data.m_mercator, POINT_COORD_BITS);
      result.emplace_back(static_cast<EventType>(data.m_eventType), mwmVersion, countryId,
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
  std::string userId;
  ServerSerializer serializer;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    userId = m_userId;
    serializer = m_serverSerializer;
  }

  for (auto it = m_metadataCache.begin(); it != m_metadataCache.end();)
  {
    std::string const url = MakeRemoteURL(m_userId, it->first.first, it->first.second);
    if (url.empty())
      return;

    std::list<Event> events = ReadEvents(it->second.m_fileName);
    if (events.empty())
    {
      ++it;
      continue;
    }

    std::string contentType = "application/octet-stream";
    std::string contentEncoding = "";
    std::vector<uint8_t> bytes = serializer != nullptr
                                     ? serializer(events, userId, contentType, contentEncoding)
                                     : SerializeForServer(events);
    ASSERT(!bytes.empty(), ());

    platform::HttpClient request(url);
#ifdef DEV_LOCAL_ADS_SERVER
    request.LoadHeaders(true);
    request.SetRawHeader("Host", "localads-statistics.maps.me");
#endif
    request.SetBodyData(std::string(bytes.begin(), bytes.end()), contentType, "POST",
                        contentEncoding);
    if (request.RunHttpRequest() && request.ErrorCode() == 200)
    {
      FileWriter::DeleteFileX(it->second.m_fileName);
      it = m_metadataCache.erase(it);
    }
    else
    {
      LOG(LWARNING, ("Sending statistics failed:", "URL:", url, "Error code:", request.ErrorCode(),
                     it->first.first, it->first.second));
      ++it;
    }
  }
  m_lastSending = std::chrono::steady_clock::now();
  m_isFirstSending = false;
}

std::vector<uint8_t> Statistics::SerializeForServer(std::list<Event> const & events) const
{
  ASSERT(!events.empty(), ());

  // TODO: implement serialization
  return std::vector<uint8_t>{1, 2, 3, 4, 5};
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

void Statistics::SetUserId(std::string const & userId)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_userId = userId;
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

void Statistics::SetCustomServerSerializer(ServerSerializer && serializer)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_serverSerializer = std::move(serializer);
}
}  // namespace local_ads
