#pragma once

#include "local_ads/event.hpp"

#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace local_ads
{
using ServerSerializer =
    std::function<std::vector<uint8_t>(std::list<Event> const & events, std::string const & userId,
                                       std::string & contentType, std::string & contentEncoding)>;

class Statistics final
{
public:
  struct PackedData
  {
    int64_t m_mercator = 0;
    uint32_t m_featureIndex = 0;
    uint32_t m_seconds = 0;
    uint16_t m_accuracy = 0;
    uint8_t m_eventType = 0;
    uint8_t m_zoomLevel = 0;
  };

  Statistics() = default;

  void Startup();

  void SetUserId(std::string const & userId);

  void SetCustomServerSerializer(ServerSerializer const & serializer);

  void RegisterEvent(Event && event);
  void RegisterEvents(std::list<Event> && events);

  std::list<Event> WriteEventsForTesting(std::list<Event> const & events,
                                         std::string & fileNameToRebuild);
  std::list<Event> ReadEventsForTesting(std::string const & fileName);
  void ProcessEventsForTesting(std::list<Event> const & events);
  void CleanupAfterTesting();

private:
  void IndexMetadata();
  void ExtractMetadata(std::string const & fileName);
  void BalanceMemory();

  std::list<Event> WriteEvents(std::list<Event> & events, std::string & fileNameToRebuild);
  std::list<Event> ReadEvents(std::string const & fileName) const;
  void ProcessEvents(std::list<Event> & events);

  void SendToServer();
  std::vector<uint8_t> SerializeForServer(std::list<Event> const & events) const;

  using MetadataKey = std::pair<std::string, int64_t>;
  struct Metadata
  {
    std::string m_fileName;
    Timestamp m_timestamp;

    Metadata() = default;
    Metadata(std::string const & fileName, Timestamp const & timestamp)
      : m_fileName(fileName), m_timestamp(timestamp)
    {
    }
  };
  std::map<MetadataKey, Metadata> m_metadataCache;

  std::string m_userId;
  ServerSerializer m_serverSerializer;
};
}  // namespace local_ads
