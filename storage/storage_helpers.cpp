#include "storage/storage_helpers.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/platform.hpp"

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

bool IsEnoughSpaceForDownload(TCountryId const & countryId, Storage const & storage)
{
  NodeAttrs nodeAttrs;
  storage.GetNodeAttrs(countryId, nodeAttrs);
  // Mwm size is less than 100 MB. In case of map update at first we download updated map and only
  // after that we do delete the obsolete map. So in such a case we might need up to 100 MB of extra
  // space.
  size_t constexpr kDownloadExtraSpaceSize = 100 /*Mb*/ * 1024 * 1024;
  size_t const downloadSpaceSize =
      kDownloadExtraSpaceSize + nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize;
  return GetPlatform().GetWritableStorageStatus(downloadSpaceSize) ==
         Platform::TStorageStatus::STORAGE_OK;
}

m2::RectD CalcLimitRect(TCountryId const & countryId,
                        Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  m2::RectD boundingBox;
  auto const accumulator =
      [&countryInfoGetter, &boundingBox](TCountryId const & descendantId, bool groupNode)
  {
    if (!groupNode)
      boundingBox.Add(countryInfoGetter.GetLimitRectForLeaf(descendantId));
  };

  storage.ForEachInSubtree(countryId, accumulator);

  ASSERT(boundingBox.IsValid(), ());
  return boundingBox;
}
} // namespace storage
