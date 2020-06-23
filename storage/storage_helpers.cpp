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
  return status == Status::DownloadFailed || status == Status::OutOfMemFailed ||
         status == Status::UnknownError;
}

bool IsEnoughSpaceForDownload(MwmSize mwmSize)
{
  // Additional size which is necessary to have on flash card to download file of mwmSize bytes.
  MwmSize constexpr kExtraSizeBytes = 10 * 1024 * 1024;
  return GetPlatform().GetWritableStorageStatus(mwmSize + kExtraSizeBytes) ==
         Platform::TStorageStatus::STORAGE_OK;
}

bool IsEnoughSpaceForDownload(MwmSize mwmSizeDiff, MwmSize maxMwmSize)
{
  // Mwm size is less than |maxMwmSize|. In case of map update at first we download updated map
  // and only after that we do delete the obsolete map. So in such a case we might need up to
  // |maxMwmSize| of extra space.
  return IsEnoughSpaceForDownload(mwmSizeDiff + maxMwmSize);
}

bool IsEnoughSpaceForDownload(CountryId const & countryId, Storage const & storage)
{
  NodeAttrs nodeAttrs;
  storage.GetNodeAttrs(countryId, nodeAttrs);
  return IsEnoughSpaceForDownload(nodeAttrs.m_mwmSize);
}

bool IsEnoughSpaceForUpdate(CountryId const & countryId, Storage const & storage)
{
  Storage::UpdateInfo updateInfo;
  
  storage.GetUpdateInfo(countryId, updateInfo);
  MwmSize spaceNeedForUpdate = updateInfo.m_sizeDifference > 0 ? updateInfo.m_sizeDifference : 0;
  return IsEnoughSpaceForDownload(spaceNeedForUpdate, storage.GetMaxMwmSizeBytes());
}

m2::RectD CalcLimitRect(CountryId const & countryId, Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  m2::RectD boundingBox;
  auto const accumulator = [&countryInfoGetter, &boundingBox](CountryId const & descendantId,
                                                              bool groupNode) {
    if (!groupNode)
      boundingBox.Add(countryInfoGetter.GetLimitRectForLeaf(descendantId));
  };

  storage.ForEachInSubtree(countryId, accumulator);

  ASSERT(boundingBox.IsValid(), ());
  return boundingBox;
}

MwmSize GetRemoteSize(diffs::DiffsDataSource const & diffsDataSource,
                      platform::CountryFile const & file)
{
  uint64_t size;
  if (diffsDataSource.SizeFor(file.GetName(), size))
    return size;
  return file.GetRemoteSize();
}
} // namespace storage
