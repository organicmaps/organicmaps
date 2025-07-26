#include "platform/mwm_traits.hpp"

namespace version
{
MwmTraits::MwmTraits(MwmVersion const & version) : m_version(version) {}

MwmTraits::SearchIndexFormat MwmTraits::GetSearchIndexFormat() const
{
  return SearchIndexFormat::CompressedBitVectorWithHeader;
}

MwmTraits::HouseToStreetTableFormat MwmTraits::GetHouseToStreetTableFormat() const
{
  return HouseToStreetTableFormat::HouseToStreetTableWithHeader;
}

MwmTraits::CentersTableFormat MwmTraits::GetCentersTableFormat() const
{
  return CentersTableFormat::EliasFanoMapWithHeader;
}

std::string DebugPrint(MwmTraits::SearchIndexFormat format)
{
  switch (format)
  {
  case MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter: return "FeaturesWithRankAndCenter";
  case MwmTraits::SearchIndexFormat::CompressedBitVector: return "CompressedBitVector";
  case MwmTraits::SearchIndexFormat::CompressedBitVectorWithHeader: return "CompressedBitVectorWithHeader";
  }
  UNREACHABLE();
}

std::string DebugPrint(MwmTraits::HouseToStreetTableFormat format)
{
  switch (format)
  {
  case MwmTraits::HouseToStreetTableFormat::HouseToStreetTableWithHeader: return "HouseToStreetTableWithHeader";
  case MwmTraits::HouseToStreetTableFormat::Unknown: return "Unknown";
  }
  UNREACHABLE();
}

std::string DebugPrint(MwmTraits::CentersTableFormat format)
{
  switch (format)
  {
  case MwmTraits::CentersTableFormat::PlainEliasFanoMap: return "PlainEliasFanoMap";
  case MwmTraits::CentersTableFormat::EliasFanoMapWithHeader: return "EliasFanoMapWithHeader";
  }
  UNREACHABLE();
}
}  // namespace version
