#include "platform/mwm_traits.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

namespace version
{
MwmTraits::MwmTraits(MwmVersion const & version) : m_version(version) {}

MwmTraits::SearchIndexFormat MwmTraits::GetSearchIndexFormat() const
{
  if (GetFormat() < version::Format::v7)
    return SearchIndexFormat::FeaturesWithRankAndCenter;
  if (GetFormat() < version::Format::v10)
    return SearchIndexFormat::CompressedBitVector;
  return SearchIndexFormat::CompressedBitVectorWithHeader;
}

MwmTraits::HouseToStreetTableFormat MwmTraits::GetHouseToStreetTableFormat() const
{
  if (GetFormat() >= version::Format::v10)
    return HouseToStreetTableFormat::HouseToStreetTableWithHeader;

  if (GetFormat() < version::Format::v7)
    return HouseToStreetTableFormat::Unknown;

  uint32_t constexpr kLastVersionWithFixed3BitsDDVector = 190113;
  if (GetVersion() <= kLastVersionWithFixed3BitsDDVector)
    return HouseToStreetTableFormat::Fixed3BitsDDVector;

  return HouseToStreetTableFormat::EliasFanoMap;
}

MwmTraits::CentersTableFormat MwmTraits::GetCentersTableFormat() const
{
  if (GetFormat() < version::Format::v9)
    return CentersTableFormat::PlainEliasFanoMap;

  uint32_t constexpr kLastVersionWithPlainEliasFanoMap = 191019;
  if (GetVersion() <= kLastVersionWithPlainEliasFanoMap)
    return CentersTableFormat::PlainEliasFanoMap;

  return CentersTableFormat::EliasFanoMapWithHeader;
}

bool MwmTraits::HasOffsetsTable() const { return GetFormat() >= version::Format::v6; }

bool MwmTraits::HasCrossMwmSection() const { return GetFormat() >= version::Format::v9; }

bool MwmTraits::HasRoutingIndex() const
{
  uint32_t constexpr kFirstVersionWithRoutingIndex = 161206;
  return GetVersion() >= kFirstVersionWithRoutingIndex;
}

bool MwmTraits::HasCuisineTypes() const
{
  uint32_t constexpr kFirstVersionWithCuisineTypes = 180917;
  return GetVersion() >= kFirstVersionWithCuisineTypes;
}

bool MwmTraits::HasIsolines() const
{
  uint32_t constexpr kFirstVersionWithIsolines = 200209;
  return GetVersion() >= kFirstVersionWithIsolines;
}

std::string DebugPrint(MwmTraits::SearchIndexFormat format)
{
  switch (format)
  {
  case MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter:
    return "FeaturesWithRankAndCenter";
  case MwmTraits::SearchIndexFormat::CompressedBitVector:
    return "CompressedBitVector";
  case MwmTraits::SearchIndexFormat::CompressedBitVectorWithHeader:
    return "CompressedBitVectorWithHeader";
  }
  UNREACHABLE();
}

std::string DebugPrint(MwmTraits::HouseToStreetTableFormat format)
{
  switch (format)
  {
  case MwmTraits::HouseToStreetTableFormat::Fixed3BitsDDVector:
    return "Fixed3BitsDDVector";
  case MwmTraits::HouseToStreetTableFormat::EliasFanoMap:
    return "EliasFanoMap";
  case MwmTraits::HouseToStreetTableFormat::HouseToStreetTableWithHeader:
    return "HouseToStreetTableWithHeader";
  case MwmTraits::HouseToStreetTableFormat::Unknown:
    return "Unknown";
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
