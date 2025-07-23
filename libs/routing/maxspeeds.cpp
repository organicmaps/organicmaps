#include "routing/maxspeeds.hpp"
#include "routing/maxspeeds_serialization.hpp"

#include "platform/measurement_utils.hpp"

#include "coding/files_container.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>

namespace routing
{
bool Maxspeeds::IsEmpty() const
{
  return m_forwardMaxspeedsTable.size() == 0 && m_bidirectionalMaxspeeds.empty();
}

Maxspeed Maxspeeds::GetMaxspeed(uint32_t fid) const
{
  // Forward only maxspeeds.
  if (HasForwardMaxspeed(fid))
  {
    auto const r = m_forwardMaxspeedsTable.rank(fid);
    CHECK_LESS(r, m_forwardMaxspeeds.Size(), ());
    uint8_t const macro = m_forwardMaxspeeds.Get(r);
    auto const speed = GetMaxspeedConverter().MacroToSpeed(static_cast<SpeedMacro>(macro));
    CHECK(speed.IsValid(), ());
    return {speed.GetUnits(), speed.GetSpeed(), kInvalidSpeed};
  }

  // Bidirectional maxspeeds.
  auto const range = std::equal_range(m_bidirectionalMaxspeeds.cbegin(), m_bidirectionalMaxspeeds.cend(), fid,
                                      FeatureMaxspeed::Less());

  if (range.second == range.first)
    return Maxspeed();  // No maxspeed for |fid| is set. Returns an invalid Maxspeed instance.

  CHECK_EQUAL(range.second - range.first, 1, ());
  return range.first->GetMaxspeed();
}

MaxspeedType Maxspeeds::GetDefaultSpeed(bool inCity, HighwayType hwType) const
{
  auto const & theMap = m_defaultSpeeds[inCity ? 1 : 0];
  auto const it = theMap.find(hwType);
  return (it != theMap.end()) ? it->second : kInvalidSpeed;
}

bool Maxspeeds::HasForwardMaxspeed(uint32_t fid) const
{
  return fid < m_forwardMaxspeedsTable.size() ? m_forwardMaxspeedsTable[fid] : false;
}

void Maxspeeds::Load(ReaderT const & reader)
{
  ReaderSource<ReaderT> src(reader);
  MaxspeedsSerializer::Deserialize(src, *this);
}

std::unique_ptr<Maxspeeds> LoadMaxspeeds(MwmSet::MwmHandle const & handle)
{
  auto const * value = handle.GetValue();
  CHECK(value, ());

  try
  {
    auto maxspeeds = std::make_unique<Maxspeeds>();
    if (value->m_cont.IsExist(MAXSPEEDS_FILE_TAG))
      maxspeeds->Load(value->m_cont.GetReader(MAXSPEEDS_FILE_TAG));
    return maxspeeds;
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("File", value->GetCountryFileName(), "Error while reading", MAXSPEEDS_FILE_TAG, "section.", e.Msg()));
    return std::make_unique<Maxspeeds>();
  }
}
}  // namespace routing
