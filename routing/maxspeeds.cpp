#include "routing/maxspeeds.hpp"

#include "routing/maxspeeds_serialization.hpp"

#include "indexer/data_source.hpp"

#include "platform/measurement_utils.hpp"

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
  if (IsEmpty())
    return Maxspeed();

  // Forward only maxspeeds.
  if (HasForwardMaxspeed(fid))
  {
    auto const r = m_forwardMaxspeedsTable.rank(fid);
    CHECK_LESS(r, m_forwardMaxspeeds.Size(), ());
    uint8_t const forwardMaxspeedMacro = m_forwardMaxspeeds.Get(r);
    CHECK(GetMaxspeedConverter().IsValidMacro(forwardMaxspeedMacro), ());
    auto const forwardMaxspeed =
        GetMaxspeedConverter().MacroToSpeed(static_cast<SpeedMacro>(forwardMaxspeedMacro));
    return {forwardMaxspeed.GetUnits(), forwardMaxspeed.GetSpeed(), kInvalidSpeed};
  }

  // Bidirectional maxspeeds.
  auto const range = std::equal_range(
      m_bidirectionalMaxspeeds.cbegin(), m_bidirectionalMaxspeeds.cend(),
      FeatureMaxspeed(fid, measurement_utils::Units::Metric, kInvalidSpeed, kInvalidSpeed),
      IsFeatureIdLess);

  if (range.second == range.first)
    return Maxspeed(); // No maxspeed for |fid| is set. Returns an invalid Maxspeed instance.

  CHECK_EQUAL(range.second - range.first, 1, ());
  return range.first->GetMaxspeed();
}

bool Maxspeeds::HasForwardMaxspeed(uint32_t fid) const
{
  return fid < m_forwardMaxspeedsTable.size() ? m_forwardMaxspeedsTable[fid] : false;
}

bool Maxspeeds::HasBidirectionalMaxspeed(uint32_t fid) const
{
  return std::binary_search(
      m_bidirectionalMaxspeeds.cbegin(), m_bidirectionalMaxspeeds.cend(),
      FeatureMaxspeed(fid, measurement_utils::Units::Metric, kInvalidSpeed, kInvalidSpeed),
      IsFeatureIdLess);
}

void LoadMaxspeeds(FilesContainerR::TReader const & reader, Maxspeeds & maxspeeds)
{
  ReaderSource<FilesContainerR::TReader> src(reader);
  MaxspeedsSerializer::Deserialize(src, maxspeeds);
}

std::unique_ptr<Maxspeeds> LoadMaxspeeds(DataSource const & dataSource,
                                         MwmSet::MwmHandle const & handle)
{
  auto maxspeeds = std::make_unique<Maxspeeds>();
  auto const value = handle.GetValue<MwmValue>();
  CHECK(value, ());
  auto const & mwmValue = *value;
  if (!mwmValue.m_cont.IsExist(MAXSPEEDS_FILE_TAG))
    return maxspeeds;

  try
  {
    LoadMaxspeeds(mwmValue.m_cont.GetReader(MAXSPEEDS_FILE_TAG), *maxspeeds);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", mwmValue.GetCountryFileName(), "Error while reading", MAXSPEEDS_FILE_TAG,
        "section.", e.Msg()));
    return std::make_unique<Maxspeeds>();
  }

  return maxspeeds;
}
}  // namespace routing
