#pragma once

#include "local_ads/event.hpp"

#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace local_ads
{
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
  ~Statistics();

  void Startup();
  void Teardown();

  void SetUserId(std::string const & userId);

  void RegisterEvent(Event && event);
  void RegisterEvents(std::list<Event> && events);

  std::list<Event> WriteEventsForTesting(std::list<Event> const & events,
                                         std::string & fileNameToRebuild);
  std::list<Event> ReadEventsForTesting(std::string const & fileName);
  void ProcessEventsForTesting(std::list<Event> const & events);
  void CleanupAfterTesting();

private:
  void ThreadRoutine();
  bool RequestEvents(std::list<Event> & events, bool & needToSend);

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
  Timestamp m_lastSending;

  std::string m_userId;

  bool m_isRunning = false;
  std::list<Event> m_events;

  std::condition_variable m_condition;
  std::mutex m_mutex;
  threads::SimpleThread m_thread;
};
}  // namespace local_ads
