#include "storage/queued_country.hpp"

#include "base/assert.hpp"

namespace storage
{
QueuedCountry::QueuedCountry(TIndex const & index, TMapOptions opt)
    : m_index(index), m_init(opt), m_left(opt), m_current(TMapOptions::ENothing)
{
  ASSERT(GetIndex().IsValid(), ("Only valid countries may be downloaded."));
  ASSERT(m_left != TMapOptions::ENothing, ("Empty file set was requested for downloading."));
  SwitchToNextFile();
}

void QueuedCountry::AddOptions(TMapOptions opt)
{
  for (TMapOptions file : {TMapOptions::EMap, TMapOptions::ECarRouting})
  {
    if (HasOptions(opt, file) && !HasOptions(m_init, file))
    {
      m_init = SetOptions(m_init, file);
      m_left = SetOptions(m_left, file);
    }
  }
}

void QueuedCountry::RemoveOptions(TMapOptions opt)
{
  for (TMapOptions file : {TMapOptions::EMap, TMapOptions::ECarRouting})
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
  // static_casts are needed here because TMapOptions values are
  // actually enum flags (see 3party/enum_flags.hpp) and bitwise
  // operators are overloaded for them.
  ASSERT(HasOptions(m_left, m_current),
         ("Current file (", m_current, ") is not specified in left files (", m_left, ")."));
  m_left = UnsetOptions(m_left, m_current);
  m_current = LeastSignificantOption(m_left);
  return m_current != TMapOptions::ENothing;
}
}  // namespace storage
