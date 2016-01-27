#include "mwm_traits.hpp"

#include "base/logging.hpp"

namespace version
{
MwmTraits::MwmTraits(version::Format versionFormat) : m_versionFormat(versionFormat) {}

MwmTraits::SearchIndexFormat MwmTraits::GetSearchIndexFormat() const
{
  if (m_versionFormat < version::Format::v7)
    return SearchIndexFormat::FeaturesWithRankAndCenter;
  return SearchIndexFormat::CompressedBitVector;
}

MwmTraits::HouseToStreetTableFormat MwmTraits::GetHouseToStreetTableFormat() const
{
  if (m_versionFormat < version::Format::v7)
    return HouseToStreetTableFormat::Unknown;
  return HouseToStreetTableFormat::Fixed3BitsDDVector;
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
}
}  // namespace version
