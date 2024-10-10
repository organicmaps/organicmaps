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
}  // namespace version
