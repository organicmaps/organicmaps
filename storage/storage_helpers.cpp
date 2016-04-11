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

bool IsEnoughSpaceForDownload(TMwmSize size)
{
  // Mwm size is less than kMaxMwmSizeBytes. In case of map update at first we download updated map
  // and only after that we do delete the obsolete map. So in such a case we might need up to
  // kMaxMwmSizeBytes of extra space.
  return GetPlatform().GetWritableStorageStatus(size + kMaxMwmSizeBytes) ==
         Platform::TStorageStatus::STORAGE_OK;
}

bool IsEnoughSpaceForDownload(TCountryId const & countryId, Storage const & storage)
{
  NodeAttrs nodeAttrs;
  storage.GetNodeAttrs(countryId, nodeAttrs);
  return IsEnoughSpaceForDownload(nodeAttrs.m_mwmSize - nodeAttrs.m_localMwmSize);
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
