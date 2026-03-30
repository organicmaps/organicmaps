#pragma once

#include "map/bookmark_helpers.hpp"

#include "search/region_address_getter.hpp"

#include "platform/safe_callback.hpp"

#include "geometry/point2d.hpp"

#include "base/macros.hpp"
#include "base/thread_checker.hpp"
#include "base/visitor.hpp"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class BookmarkManager;
class DataSource;

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

class BookmarkSorter final
{
public:
  explicit BookmarkSorter(BookmarkManager & bmManager);

  void InitRegionAddressGetter(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter);
  std::string GetLocalizedRegionAddress(m2::PointD const & pt);

  // Do not change the order.
  enum class SortingType
  {
    ByType,
    ByDistance,
    ByTime,
    ByName
  };

  struct SortedBlock
  {
    bool operator==(SortedBlock const & other) const;

    std::string m_blockName;
    kml::MarkIdCollection m_markIds;
    kml::MarkIdCollection m_trackIds;
  };
  using SortedBlocksCollection = std::vector<SortedBlock>;

  struct SortParams
  {
    enum class Status
    {
      Completed,
      Cancelled
    };

    using OnResults = std::function<void(SortedBlocksCollection && sortedBlocks, Status status)>;

    kml::MarkGroupId m_groupId = kml::kInvalidMarkGroupId;
    SortingType m_sortingType = SortingType::ByType;
    bool m_hasMyPosition = false;
    m2::PointD m_myPosition = {0.0, 0.0};
    OnResults m_onResults;
  };

  std::vector<SortingType> GetAvailableSortingTypes(kml::MarkGroupId groupId, bool hasMyPosition) const;
  void GetSortedCategory(SortParams const & params);

  bool GetLastSortingType(kml::MarkGroupId groupId, SortingType & sortingType) const;
  void SetLastSortingType(kml::MarkGroupId groupId, SortingType sortingType);
  void ResetLastSortingType(kml::MarkGroupId groupId);

  void PrepareForSearch(kml::MarkGroupId groupId);

  enum class SortedByTimeBlockType : uint32_t
  {
    WeekAgo,
    MonthAgo,
    MoreThanMonthAgo,
    MoreThanYearAgo,
    Others
  };
  static std::string GetSortedByTimeBlockName(SortedByTimeBlockType blockType);
  static std::string GetTracksSortedBlockName();
  static std::string GetBookmarksSortedBlockName();
  static std::string GetOthersSortedBlockName();
  static std::string GetNearMeSortedBlockName();

  void EnableTestMode(bool enable) { m_testModeEnabled = enable; }
  bool IsTestModeEnabled() const { return m_testModeEnabled; }

  // Metadata
  struct Properties
  {
    DECLARE_VISITOR_AND_DEBUG_PRINT(Properties, visitor(m_values, "values"))

    bool GetProperty(std::string const & propertyName, std::string & value) const;

    std::map<std::string, std::string> m_values;
  };

  struct Metadata
  {
    DECLARE_VISITOR_AND_DEBUG_PRINT(Metadata, visitor(m_entriesProperties, "entriesProperties"),
                                    visitor(m_commonProperties, "commonProperties"))

    bool GetEntryProperty(std::string const & entryName, std::string const & propertyName, std::string & value) const;

    std::map<std::string, Properties> m_entriesProperties;
    Properties m_commonProperties;
  };

  void LoadMetadata();
  void SaveMetadata();

private:
  struct SortBookmarkData
  {
    SortBookmarkData(kml::BookmarkData const & bmData, search::ReverseGeocoder::RegionAddress const & address)
      : m_id(bmData.m_id)
      , m_name(GetPreferredBookmarkName(bmData))
      , m_point(bmData.m_point)
      , m_type(GetBookmarkBaseType(bmData.m_featureTypes))
      , m_timestamp(bmData.m_timestamp)
      , m_address(address)
    {}

    kml::MarkId m_id;
    std::string m_name;
    m2::PointD m_point;
    BookmarkBaseType m_type;
    kml::Timestamp m_timestamp;
    search::ReverseGeocoder::RegionAddress m_address;
  };

  struct SortTrackData
  {
    explicit SortTrackData(kml::TrackData const & trackData)
      : m_id(trackData.m_id)
      , m_name(GetPreferredBookmarkStr(trackData.m_name))
      , m_timestamp(trackData.m_timestamp)
    {}

    kml::TrackId m_id;
    std::string m_name;
    kml::Timestamp m_timestamp;
  };

  void GetSortedCategoryImpl(SortParams const & params, std::vector<SortBookmarkData> const & bookmarksForSort,
                             std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks);

  void SortByDistance(std::vector<SortBookmarkData> const & bookmarksForSort,
                      std::vector<SortTrackData> const & tracksForSort, m2::PointD const & myPosition,
                      SortedBlocksCollection & sortedBlocks);
  static void SortByTime(std::vector<SortBookmarkData> const & bookmarksForSort,
                         std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks);
  static void SortByType(std::vector<SortBookmarkData> const & bookmarksForSort,
                         std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks);
  static void SortByName(std::vector<SortBookmarkData> const & bookmarksForSort,
                         std::vector<SortTrackData> const & tracksForSort, SortedBlocksCollection & sortedBlocks);

  using AddressesCollection = std::vector<std::pair<kml::MarkId, search::ReverseGeocoder::RegionAddress>>;
  void PrepareBookmarksAddresses(std::vector<SortBookmarkData> & bookmarksForSort, AddressesCollection & newAddresses);
  void FilterInvalidData(SortedBlocksCollection & sortedBlocks, AddressesCollection & newAddresses) const;
  void SetBookmarksAddresses(AddressesCollection const & addresses);
  static void AddTracksSortedBlock(std::vector<SortTrackData> const & sortedTracks,
                                   SortedBlocksCollection & sortedBlocks);
  static void SortTracksByTime(std::vector<SortTrackData> & tracks);
  static void SortTracksByName(std::vector<SortTrackData> & tracks);

  std::string GetMetadataEntryName(kml::MarkGroupId groupId) const;
  void CleanupInvalidMetadata();

  BookmarkManager & m_bmManager;

  std::unique_ptr<search::RegionAddressGetter> m_regionAddressGetter;
  std::mutex m_regionAddressMutex;

  Metadata m_metadata;
  bool m_testModeEnabled = false;

  DISALLOW_COPY_AND_MOVE(BookmarkSorter);
};
