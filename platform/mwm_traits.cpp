#include "mwm_traits.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

namespace version
{
MwmTraits::MwmTraits(MwmVersion const & version) : m_version(version) {}

MwmTraits::SearchIndexFormat MwmTraits::GetSearchIndexFormat() const
{
  if (GetFormat() < version::Format::v7)
    return SearchIndexFormat::FeaturesWithRankAndCenter;
  return SearchIndexFormat::CompressedBitVector;
}

MwmTraits::HouseToStreetTableFormat MwmTraits::GetHouseToStreetTableFormat() const
{
  if (GetFormat() < version::Format::v7)
    return HouseToStreetTableFormat::Unknown;
  return HouseToStreetTableFormat::Fixed3BitsDDVector;
}

bool MwmTraits::HasOffsetsTable() const { return GetFormat() >= version::Format::v6; }

bool MwmTraits::HasCrossMwmSection() const { return GetFormat() >= version::Format::v9; }

bool MwmTraits::HasRoutingIndex() const
{
  uint32_t constexpr kFirstVersionWithRoutingIndex = 161206;
  return GetVersion() >= kFirstVersionWithRoutingIndex;
}

string DebugPrint(MwmTraits::SearchIndexFormat format)
{
  switch (format)
  {
  case MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter:
    return "FeaturesWithRankAndCenter";
  case MwmTraits::SearchIndexFormat::CompressedBitVector:
    return "CompressedBitVector";
  }
  CHECK_SWITCH();
}

string DebugPrint(MwmTraits::HouseToStreetTableFormat format)
{
  switch (format)
  {
  case MwmTraits::HouseToStreetTableFormat::Fixed3BitsDDVector:
    return "Fixed3BitsDDVector";
  case MwmTraits::HouseToStreetTableFormat::Unknown:
    return "Unknown";
  }
  CHECK_SWITCH();
}
}  // namespace version
