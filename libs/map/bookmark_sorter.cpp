#include "map/bookmark_sorter.hpp"
#include "map/bookmark_manager.hpp"
#include "map/search_api.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/serdes_json.hpp"

#include "base/file_name_utils.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <chrono>
#include <set>

namespace
{
std::string const kMetadataFileName = "bm.json";
std::string const kSortingTypeProperty = "sortingType";
size_t const kMinCommonTypesCount = 3;
double const kNearDistanceInMeters = 20 * 1000.0;

std::string ToString(BookmarkSorter::SortingType type)
{
  switch (type)
  {
  case BookmarkSorter::SortingType::ByTime: return "ByTime";
  case BookmarkSorter::SortingType::ByType: return "ByType";
  case BookmarkSorter::SortingType::ByDistance: return "ByDistance";
  case BookmarkSorter::SortingType::ByName: return "ByName";
  }
  UNREACHABLE();
}

bool GetSortingType(std::string const & typeStr, BookmarkSorter::SortingType & type)
{
  if (typeStr == ToString(BookmarkSorter::SortingType::ByTime))
    type = BookmarkSorter::SortingType::ByTime;
  else if (typeStr == ToString(BookmarkSorter::SortingType::ByType))
    type = BookmarkSorter::SortingType::ByType;
  else if (typeStr == ToString(BookmarkSorter::SortingType::ByDistance))
    type = BookmarkSorter::SortingType::ByDistance;
  else if (typeStr == ToString(BookmarkSorter::SortingType::ByName))
    type = BookmarkSorter::SortingType::ByName;
  else
    return false;
  return true;
}

template <typename T, typename R>
BookmarkSorter::SortedByTimeBlockType GetSortedByTimeBlockType(std::chrono::duration<T, R> const & timePeriod)
{
  auto constexpr kDay = std::chrono::hours(24);
  auto constexpr kWeek = 7 * kDay;
  auto constexpr kMonth = 31 * kDay;
  auto constexpr kYear = 365 * kDay;

  if (timePeriod < kWeek)
    return BookmarkSorter::SortedByTimeBlockType::WeekAgo;
  if (timePeriod < kMonth)
    return BookmarkSorter::SortedByTimeBlockType::MonthAgo;
  if (timePeriod < kYear)
    return BookmarkSorter::SortedByTimeBlockType::MoreThanMonthAgo;
  return BookmarkSorter::SortedByTimeBlockType::MoreThanYearAgo;
}
}  // namespace

bool BookmarkSorter::Properties::GetProperty(std::string const & propertyName, std::string & value) const
{
  auto const it = m_values.find(propertyName);
  if (it == m_values.end())
    return false;
  value = it->second;
  return true;
}

bool BookmarkSorter::Metadata::GetEntryProperty(std::string const & entryName, std::string const & propertyName,
                                                std::string & value) const
{
  auto const it = m_entriesProperties.find(entryName);
  if (it == m_entriesProperties.end())
    return false;
  return it->second.GetProperty(propertyName, value);
}

bool BookmarkSorter::SortedBlock::operator==(SortedBlock const & other) const
{
  return m_blockName == other.m_blockName && m_markIds == other.m_markIds && m_trackIds == other.m_trackIds;
}

BookmarkSorter::BookmarkSorter(BookmarkManager & bmManager) : m_bmManager(bmManager) {}

void BookmarkSorter::InitRegionAddressGetter(DataSource const & dataSource,
                                             storage::CountryInfoGetter const & infoGetter)
{
  std::unique_lock const lock(m_regionAddressMutex);
  m_regionAddressGetter = std::make_unique<search::RegionAddressGetter>(dataSource, infoGetter);
}

std::string BookmarkSorter::GetLocalizedRegionAddress(m2::PointD const & pt)
{
  CHECK(m_testModeEnabled, ());

  std::unique_lock const lock(m_regionAddressMutex);
  if (m_regionAddressGetter == nullptr)
  {
    LOG(LWARNING, ("Region address getter is not set. Address getting failed."));
    return {};
  }
  return m_regionAddressGetter->GetLocalizedRegionAddress(pt);
}

std::vector<BookmarkSorter::SortingType> BookmarkSorter::GetAvailableSortingTypes(kml::MarkGroupId groupId,
                                                                                  bool hasMyPosition) const
{
  ASSERT(BookmarkManager::IsBookmarkCategory(groupId), ());

  auto const * group = m_bmManager.GetGroup(groupId);

  bool byTypeChecked = false;
  bool byTimeChecked = false;

  std::map<BookmarkBaseType, size_t> typesCount;
  for (auto markId : group->GetUserMarks())
  {
    auto const & bookmarkData = m_bmManager.GetBookmark(markId)->GetData();

    if (!byTypeChecked && !bookmarkData.m_featureTypes.empty())
    {
      auto const type = GetBookmarkBaseType(bookmarkData.m_featureTypes);
      if (type == BookmarkBaseType::Hotel)
      {
        byTypeChecked = true;
      }
      else if (type != BookmarkBaseType::None)
      {
        auto const count = ++typesCount[type];
        byTypeChecked = (count == kMinCommonTypesCount);
      }
    }

    if (!byTimeChecked)
      byTimeChecked = !kml::IsEqual(bookmarkData.m_timestamp, kml::Timestamp{});

    if (byTypeChecked && byTimeChecked)
      break;
  }

  if (!byTimeChecked)
  {
    for (auto trackId : group->GetUserLines())
    {
      if (!kml::IsEqual(m_bmManager.GetTrack(trackId)->GetData().m_timestamp, kml::Timestamp{}))
      {
        byTimeChecked = true;
        break;
      }
    }
  }

  std::vector<SortingType> sortingTypes;
  if (byTypeChecked)
    sortingTypes.push_back(SortingType::ByType);
  if (hasMyPosition && !group->GetUserMarks().empty())
    sortingTypes.push_back(SortingType::ByDistance);
  if (byTimeChecked)
    sortingTypes.push_back(SortingType::ByTime);
  sortingTypes.push_back(SortingType::ByName);
  return sortingTypes;
}

void BookmarkSorter::GetSortedCategory(SortParams const & params)
{
  CHECK(params.m_onResults != nullptr, ());

  auto const * group = m_bmManager.GetGroup(params.m_groupId);

  std::vector<SortBookmarkData> bookmarksForSort;
  bookmarksForSort.reserve(group->GetUserMarks().size());
  for (auto markId : group->GetUserMarks())
  {
    auto const * bm = m_bmManager.GetBookmark(markId);
    bookmarksForSort.emplace_back(bm->GetData(), bm->GetAddress());
  }

  std::vector<SortTrackData> tracksForSort;
  tracksForSort.reserve(group->GetUserLines().size());
  for (auto trackId : group->GetUserLines())
  {
    auto const * track = m_bmManager.GetTrack(trackId);
    tracksForSort.emplace_back(track->GetData());
  }

  if (m_testModeEnabled)
  {
    std::unique_lock const lock(m_regionAddressMutex);
    if (m_regionAddressGetter == nullptr)
    {
      LOG(LWARNING, ("Region address getter is not set, bookmarks sorting failed."));
      params.m_onResults({}, SortParams::Status::Cancelled);
      return;
    }
    AddressesCollection newAddresses;
    if (params.m_sortingType == SortingType::ByDistance)
      PrepareBookmarksAddresses(bookmarksForSort, newAddresses);

    SortedBlocksCollection sortedBlocks;
    GetSortedCategoryImpl(params, bookmarksForSort, tracksForSort, sortedBlocks);
    params.m_onResults(std::move(sortedBlocks), SortParams::Status::Completed);
    return;
  }

  GetPlatform().RunTask(Platform::Thread::Background, [this, params, bookmarksForSort = std::move(bookmarksForSort),
                                                       tracksForSort = std::move(tracksForSort)]() mutable
  {
    std::unique_lock const lock(m_regionAddressMutex);
    if (m_regionAddressGetter == nullptr)
    {
      GetPlatform().RunTask(Platform::Thread::Gui, [params]
      {
        LOG(LWARNING, ("Region address getter is not set, bookmarks sorting failed."));
        params.m_onResults({}, SortParams::Status::Cancelled);
      });
      return;
    }

    AddressesCollection newAddresses;
    if (params.m_sortingType == SortingType::ByDistance)
      PrepareBookmarksAddresses(bookmarksForSort, newAddresses);

    SortedBlocksCollection sortedBlocks;
    GetSortedCategoryImpl(params, bookmarksForSort, tracksForSort, sortedBlocks);

    GetPlatform().RunTask(Platform::Thread::Gui, [this, params, newAddresses = std::move(newAddresses),
                                                  sortedBlocks = std::move(sortedBlocks)]() mutable
    {
      FilterInvalidData(sortedBlocks, newAddresses);
      if (sortedBlocks.empty())
      {
        params.m_onResults({}, SortParams::Status::Cancelled);
        return;
      }
      SetBookmarksAddresses(newAddresses);
      params.m_onResults(std::move(sortedBlocks), SortParams::Status::Completed);
    });
  });
}

bool BookmarkSorter::GetLastSortingType(kml::MarkGroupId groupId, SortingType & sortingType) const
{
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return false;

  std::string sortingTypeStr;
  if (m_metadata.GetEntryProperty(entryName, kSortingTypeProperty, sortingTypeStr))
    return GetSortingType(sortingTypeStr, sortingType);
  return false;
}

void BookmarkSorter::SetLastSortingType(kml::MarkGroupId groupId, SortingType sortingType)
{
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return;

  m_metadata.m_entriesProperties[entryName].m_values[kSortingTypeProperty] = ToString(sortingType);
  SaveMetadata();
}

void BookmarkSorter::ResetLastSortingType(kml::MarkGroupId groupId)
{
  auto const entryName = GetMetadataEntryName(groupId);
  if (entryName.empty())
    return;

  m_metadata.m_entriesProperties[entryName].m_values.erase(kSortingTypeProperty);
  SaveMetadata();
}

void BookmarkSorter::PrepareForSearch(kml::MarkGroupId groupId)
{
  m_bmManager.PrepareForSearch(groupId);
}

// static
std::string BookmarkSorter::GetSortedByTimeBlockName(SortedByTimeBlockType blockType)
{
  switch (blockType)
  {
  case SortedByTimeBlockType::WeekAgo: return platform::GetLocalizedString("week_ago_sorttype");
  case SortedByTimeBlockType::MonthAgo: return platform::GetLocalizedString("month_ago_sorttype");
  case SortedByTimeBlockType::MoreThanMonthAgo: return platform::GetLocalizedString("moremonth_ago_sorttype");
  case SortedByTimeBlockType::MoreThanYearAgo: return platform::GetLocalizedString("moreyear_ago_sorttype");
  case SortedByTimeBlockType::Others: return GetOthersSortedBlockName();
  }
  UNREACHABLE();
}

// static
std::string BookmarkSorter::GetTracksSortedBlockName()
{
  return platform::GetLocalizedString("tracks_title");
}

// static
std::string BookmarkSorter::GetBookmarksSortedBlockName()
{
  return platform::GetLocalizedString("bookmarks");
}

// static
std::string BookmarkSorter::GetOthersSortedBlockName()
{
  return platform::GetLocalizedString("others_sorttype");
}

// static
std::string BookmarkSorter::GetNearMeSortedBlockName()
{
  return platform::GetLocalizedString("near_me_sorttype");
}

void BookmarkSorter::GetSortedCategoryImpl(SortParams const & params,
                                           std::vector<SortBookmarkData> const & bookmarksForSort,
                                           std::vector<SortTrackData> const & tracksForSort,
                                           SortedBlocksCollection & sortedBlocks)
{
  switch (params.m_sortingType)
  {
  case SortingType::ByDistance:
    CHECK(params.m_hasMyPosition, ());
    SortByDistance(bookmarksForSort, tracksForSort, params.m_myPosition, sortedBlocks);
    return;
  case SortingType::ByTime: SortByTime(bookmarksForSort, tracksForSort, sortedBlocks); return;
  case SortingType::ByType: SortByType(bookmarksForSort, tracksForSort, sortedBlocks); return;
  case SortingType::ByName: SortByName(bookmarksForSort, tracksForSort, sortedBlocks); return;
  }
  UNREACHABLE();
}

void BookmarkSorter::SortByDistance(std::vector<SortBookmarkData> const & bookmarksForSort,
                                    std::vector<SortTrackData> const & tracksForSort, m2::PointD const & myPosition,
                                    SortedBlocksCollection & sortedBlocks)
{
  CHECK(m_regionAddressGetter != nullptr, ());

  AddTracksSortedBlock(tracksForSort, sortedBlocks);

  std::vector<std::pair<SortBookmarkData const *, double>> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
  {
    auto const distance = mercator::DistanceOnEarth(mark.m_point, myPosition);
    sortedMarks.emplace_back(&mark, distance);
  }

  std::sort(sortedMarks.begin(), sortedMarks.end(),
            [](std::pair<SortBookmarkData const *, double> const & lbm,
               std::pair<SortBookmarkData const *, double> const & rbm) { return lbm.second < rbm.second; });

  std::map<search::ReverseGeocoder::RegionAddress, size_t> regionBlockIndices;
  SortedBlock othersBlock;
  for (auto markItem : sortedMarks)
  {
    auto const & mark = *markItem.first;
    auto const distance = markItem.second;

    if (!mark.m_address.IsValid())
    {
      othersBlock.m_markIds.push_back(mark.m_id);
      continue;
    }

    auto const currentRegion =
        distance < kNearDistanceInMeters ? search::ReverseGeocoder::RegionAddress() : mark.m_address;

    size_t blockIndex;
    auto const it = regionBlockIndices.find(currentRegion);
    if (it == regionBlockIndices.end())
    {
      SortedBlock regionBlock;
      if (currentRegion.IsValid())
        regionBlock.m_blockName = m_regionAddressGetter->GetLocalizedRegionAddress(currentRegion);
      else
        regionBlock.m_blockName = GetNearMeSortedBlockName();

      blockIndex = sortedBlocks.size();
      regionBlockIndices[currentRegion] = blockIndex;
      sortedBlocks.push_back(regionBlock);
    }
    else
    {
      blockIndex = it->second;
    }

    sortedBlocks[blockIndex].m_markIds.push_back(mark.m_id);
  }

  if (!othersBlock.m_markIds.empty())
  {
    othersBlock.m_blockName = GetOthersSortedBlockName();
    sortedBlocks.emplace_back(std::move(othersBlock));
  }
}

// static
void BookmarkSorter::SortByTime(std::vector<SortBookmarkData> const & bookmarksForSort,
                                std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks)
{
  std::vector<SortTrackData> sortedTracks = tracksForSort;
  SortTracksByTime(sortedTracks);
  AddTracksSortedBlock(sortedTracks, sortedBlocks);

  std::vector<SortBookmarkData const *> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
    sortedMarks.push_back(&mark);

  std::sort(sortedMarks.begin(), sortedMarks.end(), [](SortBookmarkData const * lbm, SortBookmarkData const * rbm)
  { return lbm->m_timestamp > rbm->m_timestamp; });

  auto const currentTime = kml::TimestampClock::now();

  std::optional<SortedByTimeBlockType> lastBlockType;
  SortedBlock currentBlock;
  for (auto mark : sortedMarks)
  {
    auto currentBlockType = SortedByTimeBlockType::Others;
    if (mark->m_timestamp != kml::Timestamp{})
      currentBlockType = ::GetSortedByTimeBlockType(currentTime - mark->m_timestamp);

    if (!lastBlockType)
    {
      lastBlockType = currentBlockType;
      currentBlock.m_blockName = GetSortedByTimeBlockName(currentBlockType);
    }

    if (currentBlockType != *lastBlockType)
    {
      sortedBlocks.push_back(currentBlock);
      currentBlock = SortedBlock();
      currentBlock.m_blockName = GetSortedByTimeBlockName(currentBlockType);
    }
    lastBlockType = currentBlockType;
    currentBlock.m_markIds.push_back(mark->m_id);
  }
  if (!currentBlock.m_markIds.empty())
    sortedBlocks.push_back(currentBlock);
}

// static
void BookmarkSorter::SortByType(std::vector<SortBookmarkData> const & bookmarksForSort,
                                std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks)
{
  AddTracksSortedBlock(tracksForSort, sortedBlocks);

  std::vector<SortBookmarkData const *> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
    sortedMarks.push_back(&mark);

  std::sort(sortedMarks.begin(), sortedMarks.end(), [](SortBookmarkData const * lbm, SortBookmarkData const * rbm)
  { return lbm->m_timestamp > rbm->m_timestamp; });

  std::map<BookmarkBaseType, size_t> typesCount;
  size_t othersTypeMarksCount = 0;
  for (auto mark : sortedMarks)
  {
    auto const type = mark->m_type;
    if (type == BookmarkBaseType::None)
    {
      ++othersTypeMarksCount;
      continue;
    }

    ++typesCount[type];
  }

  std::vector<std::pair<BookmarkBaseType, size_t>> sortedTypes;
  for (auto const & typeCount : typesCount)
    if (typeCount.second < kMinCommonTypesCount && typeCount.first != BookmarkBaseType::Hotel)
      othersTypeMarksCount += typeCount.second;
    else
      sortedTypes.emplace_back(typeCount);

  std::sort(sortedTypes.begin(), sortedTypes.end(),
            [](std::pair<BookmarkBaseType, size_t> const & l, std::pair<BookmarkBaseType, size_t> const & r)
  { return l.second > r.second; });

  std::map<BookmarkBaseType, size_t> blockIndices;
  sortedBlocks.reserve(sortedBlocks.size() + sortedTypes.size() + (othersTypeMarksCount > 0 ? 1 : 0));
  for (auto const & sortedType : sortedTypes)
  {
    auto const type = sortedType.first;
    SortedBlock typeBlock;
    typeBlock.m_blockName = GetLocalizedBookmarkBaseType(type);
    typeBlock.m_markIds.reserve(sortedType.second);
    blockIndices[type] = sortedBlocks.size();
    sortedBlocks.emplace_back(std::move(typeBlock));
  }

  if (othersTypeMarksCount > 0)
  {
    SortedBlock othersBlock;
    othersBlock.m_blockName = GetOthersSortedBlockName();
    othersBlock.m_markIds.reserve(othersTypeMarksCount);
    sortedBlocks.emplace_back(std::move(othersBlock));
  }

  for (auto mark : sortedMarks)
  {
    auto const type = mark->m_type;
    if (type == BookmarkBaseType::None || (type != BookmarkBaseType::Hotel && typesCount[type] < kMinCommonTypesCount))
      sortedBlocks.back().m_markIds.push_back(mark->m_id);
    else
      sortedBlocks[blockIndices[type]].m_markIds.push_back(mark->m_id);
  }
}

// static
void BookmarkSorter::SortByName(std::vector<SortBookmarkData> const & bookmarksForSort,
                                std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks)
{
  std::vector<SortTrackData> sortedTracks = tracksForSort;
  SortTracksByName(sortedTracks);
  AddTracksSortedBlock(sortedTracks, sortedBlocks);

  std::vector<SortBookmarkData const *> sortedMarks;
  sortedMarks.reserve(bookmarksForSort.size());
  for (auto const & mark : bookmarksForSort)
    sortedMarks.push_back(&mark);

  std::sort(sortedMarks.begin(), sortedMarks.end(),
            [](SortBookmarkData const * lbm, SortBookmarkData const * rbm) { return lbm->m_name < rbm->m_name; });

  SortedBlock bookmarkBlock;
  bookmarkBlock.m_blockName = GetBookmarksSortedBlockName();
  for (auto mark : sortedMarks)
    bookmarkBlock.m_markIds.push_back(mark->m_id);
  sortedBlocks.push_back(bookmarkBlock);
}

void BookmarkSorter::PrepareBookmarksAddresses(std::vector<SortBookmarkData> & bookmarksForSort,
                                               AddressesCollection & newAddresses)
{
  CHECK(m_regionAddressGetter != nullptr, ());

  for (auto & markData : bookmarksForSort)
  {
    if (!markData.m_address.IsValid())
    {
      markData.m_address = m_regionAddressGetter->GetNearbyRegionAddress(markData.m_point);
      if (markData.m_address.IsValid())
        newAddresses.emplace_back(markData.m_id, markData.m_address);
    }
  }
}

void BookmarkSorter::FilterInvalidData(SortedBlocksCollection & sortedBlocks, AddressesCollection & newAddresses) const
{
  for (auto & block : sortedBlocks)
  {
    m_bmManager.FilterInvalidBookmarks(block.m_markIds);
    m_bmManager.FilterInvalidTracks(block.m_trackIds);
  }

  base::EraseIf(sortedBlocks,
                [](SortedBlock const & block) { return block.m_trackIds.empty() && block.m_markIds.empty(); });

  base::EraseIf(newAddresses, [this](std::pair<kml::MarkId, search::ReverseGeocoder::RegionAddress> const & item)
  { return m_bmManager.GetBookmark(item.first) == nullptr; });
}

void BookmarkSorter::SetBookmarksAddresses(AddressesCollection const & addresses)
{
  auto session = m_bmManager.GetEditSession();
  for (auto const & item : addresses)
  {
    auto bm = session.GetBookmarkForEdit(item.first);
    bm->SetAddress(item.second);
  }
}

// static
void BookmarkSorter::AddTracksSortedBlock(std::vector<SortTrackData> const & sortedTracks,
                                          SortedBlocksCollection & sortedBlocks)
{
  if (!sortedTracks.empty())
  {
    SortedBlock tracksBlock;
    tracksBlock.m_blockName = GetTracksSortedBlockName();
    tracksBlock.m_trackIds.reserve(sortedTracks.size());
    for (auto const & track : sortedTracks)
      tracksBlock.m_trackIds.push_back(track.m_id);
    sortedBlocks.emplace_back(std::move(tracksBlock));
  }
}

// static
void BookmarkSorter::SortTracksByTime(std::vector<SortTrackData> & tracks)
{
  bool hasTimestamp = false;
  for (auto const & track : tracks)
  {
    if (!kml::IsEqual(track.m_timestamp, kml::Timestamp{}))
    {
      hasTimestamp = true;
      break;
    }
  }

  if (!hasTimestamp)
    return;

  std::sort(tracks.begin(), tracks.end(),
            [](SortTrackData const & lbm, SortTrackData const & rbm) { return lbm.m_timestamp > rbm.m_timestamp; });
}

// static
void BookmarkSorter::SortTracksByName(std::vector<SortTrackData> & tracks)
{
  std::sort(tracks.begin(), tracks.end(),
            [](SortTrackData const & lbm, SortTrackData const & rbm) { return lbm.m_name < rbm.m_name; });
}

std::string BookmarkSorter::GetMetadataEntryName(kml::MarkGroupId groupId) const
{
  CHECK(BookmarkManager::IsBookmarkCategory(groupId), ());
  return m_bmManager.GetCategoryFileName(groupId);
}

void BookmarkSorter::CleanupInvalidMetadata()
{
  std::set<std::string> activeEntries;
  for (auto groupId : m_bmManager.GetUnsortedBmGroupsIdList())
  {
    auto const entryName = GetMetadataEntryName(groupId);
    if (!entryName.empty())
      activeEntries.insert(entryName);
  }

  auto it = m_metadata.m_entriesProperties.begin();
  while (it != m_metadata.m_entriesProperties.end())
    if (activeEntries.find(it->first) == activeEntries.end() || it->second.m_values.empty())
      it = m_metadata.m_entriesProperties.erase(it);
    else
      ++it;
}

void BookmarkSorter::SaveMetadata()
{
  CleanupInvalidMetadata();

  auto const metadataFilePath = base::JoinPath(GetPlatform().WritableDir(), kMetadataFileName);
  std::string jsonStr;
  {
    using Sink = MemWriter<std::string>;
    Sink sink(jsonStr);
    coding::SerializerJson<Sink> ser(sink);
    ser(m_metadata);
  }

  try
  {
    FileWriter w(metadataFilePath);
    w.Write(jsonStr.c_str(), jsonStr.length());
  }
  catch (FileWriter::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while writing file:", metadataFilePath, "reason:", exception.what()));
  }
}

void BookmarkSorter::LoadMetadata()
{
  auto const metadataFilePath = base::JoinPath(GetPlatform().WritableDir(), kMetadataFileName);
  if (!Platform::IsFileExistsByFullPath(metadataFilePath))
    return;

  Metadata metadata;
  std::string json;

  try
  {
    FileReader(metadataFilePath).ReadAsString(json);

    if (json.empty())
      return;

    coding::DeserializerJson des(json);
    des(metadata);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while reading file:", metadataFilePath, "reason:", exception.what()));
    return;
  }
  catch (base::Json::Exception const & exception)
  {
    LOG(LWARNING, ("Exception while parsing file:", metadataFilePath, "reason:", exception.what(), "json:", json));
    return;
  }

  m_metadata = metadata;
}
