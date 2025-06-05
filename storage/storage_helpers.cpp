#include "storage/storage_helpers.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "std/target_os.hpp"

namespace storage
{
bool IsPointCoveredByDownloadedMaps(m2::PointD const & position, Storage const & storage,
                                    CountryInfoGetter const & countryInfoGetter)
{
  return storage.IsNodeDownloaded(countryInfoGetter.GetRegionCountryId(position));
}

bool IsDownloadFailed(Status status)
{
  return status == Status::DownloadFailed || status == Status::OutOfMemFailed || status == Status::UnknownError;
}

bool IsEnoughSpaceForDownload(MwmSize mwmSize)
{
  // Additional size which is necessary to have on flash card to download file of mwmSize bytes.
  MwmSize constexpr kExtraSizeBytes = 10 * 1024 * 1024;
  return GetPlatform().GetWritableStorageStatus(mwmSize + kExtraSizeBytes) == Platform::TStorageStatus::STORAGE_OK;
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

  /// @todo Review this logic when Storage::ApplyDiff will be restored.

  // 1. For unlimited concurrent downloading process with "download and apply diff" strategy:
  // - download and save all MWMs or Diffs = m_totalDownloadSizeInBytes
  // - max MWM file size to apply diff patch (patches are applying one-by-one) = m_maxFileSizeInBytes
  // - final size difference between old and new MWMs = m_sizeDifference

  [[maybe_unused]] MwmSize const diff = updateInfo.m_sizeDifference > 0 ? updateInfo.m_sizeDifference : 0;
  //  return IsEnoughSpaceForDownload(std::max(diff, updateInfo.m_totalDownloadSizeInBytes) +
  //                                  updateInfo.m_maxFileSizeInBytes);

  // 2. For the current "download and replace" strategy:
  // - Android and Desktop has 1 simultaneous download
  // - iOS has unlimited simultaneous downloads
#ifdef OMIM_OS_IPHONE
  return IsEnoughSpaceForDownload(updateInfo.m_totalDownloadSizeInBytes);
#else
  return IsEnoughSpaceForDownload(diff + updateInfo.m_maxFileSizeInBytes);
#endif  // OMIM_OS_IPHONE
}

m2::RectD CalcLimitRect(CountryId const & countryId, Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  m2::RectD boundingBox;
  auto const accumulator = [&countryInfoGetter, &boundingBox](CountryId const & descendantId, bool groupNode)
  {
    if (!groupNode)
      boundingBox.Add(countryInfoGetter.GetLimitRectForLeaf(descendantId));
  };

  storage.ForEachInSubtree(countryId, accumulator);

  ASSERT(boundingBox.IsValid(), ());
  return boundingBox;
}

MwmSize GetRemoteSize(diffs::DiffsDataSource const & diffsDataSource, platform::CountryFile const & file)
{
  uint64_t size;
  if (diffsDataSource.SizeFor(file.GetName(), size))
    return size;
  return file.GetRemoteSize();
}
}  // namespace storage
