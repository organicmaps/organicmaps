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

std::string_view DebugPrint(MwmTraits::SearchIndexFormat format)
{
  switch (format)
  {
    using enum MwmTraits::SearchIndexFormat;
  case CompressedBitVectorWithHeader: return "CompressedBitVectorWithHeader";
  }
  UNREACHABLE();
}

std::string_view DebugPrint(MwmTraits::HouseToStreetTableFormat format)
{
  switch (format)
  {
    using enum MwmTraits::HouseToStreetTableFormat;
  case HouseToStreetTableWithHeader: return "HouseToStreetTableWithHeader";
  case Unknown: return "Unknown";
  }
  UNREACHABLE();
}

std::string_view DebugPrint(MwmTraits::CentersTableFormat format)
{
  switch (format)
  {
    using enum MwmTraits::CentersTableFormat;
  case EliasFanoMapWithHeader: return "EliasFanoMapWithHeader";
  }
  UNREACHABLE();
}
}  // namespace version
