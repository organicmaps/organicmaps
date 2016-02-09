#include "storage/storage_helpers.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

namespace storage
{
bool IsPointCoveredByDownloadedMaps(m2::PointD const & position,
                                    Storage const & storage,
                                    CountryInfoGetter const & countryInfoGetter)
{
  return storage.IsNodeDownloaded(countryInfoGetter.GetRegionCountryId(position));
}

bool IsDownloadFailed(Status status)
{
  return status == Status::EDownloadFailed || status == Status::EOutOfMemFailed ||
         status == Status::EUnknown;
}

m2::RectD CalcLimitRect(TCountryId const & countryId,
                        Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  m2::RectD boundingBox;
  auto const accumulator =
      [&countryInfoGetter, &boundingBox](TCountryId const & descendantId, bool expandableNode)
  {
    if (!expandableNode)
      boundingBox.Add(countryInfoGetter.GetLimitRectForLeaf(descendantId));
  };

  storage.ForEachInSubtree(countryId, accumulator);

  ASSERT(boundingBox.IsValid(), ());
  return boundingBox;
}
} // namespace storage
