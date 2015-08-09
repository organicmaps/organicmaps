#include "storage/queued_country.hpp"

#include "base/assert.hpp"

namespace storage
{
QueuedCountry::QueuedCountry(TIndex const & index, MapOptions opt)
    : m_index(index), m_init(opt), m_left(opt), m_current(MapOptions::Nothing)
{
  ASSERT(GetIndex().IsValid(), ("Only valid countries may be downloaded."));
  ASSERT(m_left != MapOptions::Nothing, ("Empty file set was requested for downloading."));
  SwitchToNextFile();
}

void QueuedCountry::AddOptions(MapOptions opt)
{
  for (MapOptions file : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (HasOptions(opt, file) && !HasOptions(m_init, file))
    {
      m_init = SetOptions(m_init, file);
      m_left = SetOptions(m_left, file);
    }
  }
}

void QueuedCountry::RemoveOptions(MapOptions opt)
{
  for (MapOptions file : {MapOptions::Map, MapOptions::CarRouting})
  {
    if (HasOptions(opt, file) && HasOptions(m_init, file))
    {
      m_init = UnsetOptions(m_init, file);
      m_left = UnsetOptions(m_left, file);
    }
  }
  if (HasOptions(opt, m_current))
    m_current = LeastSignificantOption(m_left);
}

bool QueuedCountry::SwitchToNextFile()
{
  ASSERT(HasOptions(m_left, m_current),
         ("Current file (", m_current, ") is not specified in left files (", m_left, ")."));
  m_left = UnsetOptions(m_left, m_current);
  m_current = LeastSignificantOption(m_left);
  return m_current != MapOptions::Nothing;
}
}  // namespace storage
