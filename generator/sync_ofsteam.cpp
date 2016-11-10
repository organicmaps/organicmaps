#include "sync_ofsteam.hpp"

#include "std/iostream.hpp"

namespace generator
{
void SyncOfstream::Open(string const & fullPath)
{
  lock_guard<mutex> guard(m_mutex);
  m_stream.open(fullPath, std::ofstream::out);
}

bool SyncOfstream::IsOpened()
{
  lock_guard<mutex> guard(m_mutex);
  return m_stream.is_open() && !m_stream.fail();
}

void SyncOfstream::Write(uint32_t featureId, vector<osm::Id> const & osmIds)
{
  if (!IsOpened())
    return;

  lock_guard<mutex> guard(m_mutex);
  m_stream << featureId;
  for (osm::Id const & osmId : osmIds)
    m_stream << "," << osmId.OsmId();
  m_stream << endl;
}
}  // namespace generator
