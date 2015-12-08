#include "search/mwm_traits.hpp"

#include "base/logging.hpp"

namespace search
{
MwmTraits::MwmTraits(version::Format versionFormat) : m_versionFormat(versionFormat) {}

MwmTraits::SearchIndexFormat MwmTraits::GetSearchIndexFormat() const
{
  if (m_versionFormat < version::v7)
    return SearchIndexFormat::FeaturesWithRankAndCenter;
  if (m_versionFormat == version::v7)
    return SearchIndexFormat::CompressedBitVector;

  LOG(LWARNING, ("Unknown search index format."));
  return SearchIndexFormat::Unknown;
}

MwmTraits::HouseToStreetTableFormat MwmTraits::GetHouseToStreetTableFormat() const
{
  if (m_versionFormat < version::v7)
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
  case MwmTraits::SearchIndexFormat::Unknown:
    return "Unknown";
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
}  // namespace search
