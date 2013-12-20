#pragma once

#include "../../platform/platform.hpp"

namespace stats
{

class EventWriter;

class EventTracker
{
public:
  EventTracker();
  ~EventTracker();
  bool TrackSearch(string const & query);

private:
  EventWriter * m_writer;
};

}  // namespace stats
