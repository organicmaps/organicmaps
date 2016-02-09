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

m2::RectD CalcLimitRect(TCountryId countryId,
                        Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  m2::RectD boundingBox;
  auto const accumulater =
      [&countryInfoGetter, &boundingBox](TCountryId const & descendantCountryId, bool expandableNode)
  {
    if (!expandableNode)
      boundingBox.Add(countryInfoGetter.CalcLimitRectForLeaf(descendantCountryId));
  };

  storage.ForEachInSubtree(countryId, accumulater);

  ASSERT(boundingBox.IsValid(), ());
  return boundingBox;
}
} // namespace storage
