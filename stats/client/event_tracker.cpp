#include "event_tracker.hpp"
#include "event_writer.hpp"

#include "../common/wire.pb.h"

namespace stats
{

EventTracker::EventTracker()
  : m_writer(new EventWriter(GetPlatform().UniqueClientId(), GetPlatform().WritableDir() + "stats"))
{
}

EventTracker::~EventTracker()
{
  delete m_writer;
}

bool EventTracker::TrackSearch(string const & query)
{
  class Search s;
  s.set_query(query);
  return m_writer->Write(s);
}

}  // namespace stats
