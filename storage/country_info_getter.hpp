#pragma once

#include "storage/country_decl.hpp"
#include "storage/storage_defines.hpp"

#include "platform/platform.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include "coding/files_container.hpp"

#include "base/cache.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace storage
{
// This class allows users to get information about country by point or by name.
class CountryInfoGetterBase
{
public:
  // Identifier of a region (index in m_countries array).
  using RegionId = size_t;
  using RegionIdVec = std::vector<RegionId>;

  virtual ~CountryInfoGetterBase() = default;

  // Returns country file name without an extension for a country |pt|
  // belongs to. If there is no such country, returns an empty
  // string.
  CountryId GetRegionCountryId(m2::PointD const & pt) const;

  // Returns true when |pt| belongs to at least one of the specified
  // |regions|.
  bool BelongsToAnyRegion(m2::PointD const & pt, RegionIdVec const & regions) const;

  // Returns true if there's at least one region with id equal to |countryId|.
  bool BelongsToAnyRegion(CountryId const & countryId, RegionIdVec const & regions) const;

  std::vector<CountryDef> const & GetCountries() const { return m_countries; }

protected:
  // Returns identifier of the first country containing |pt| or |kInvalidId| if there is none.
  RegionId FindFirstCountry(m2::PointD const & pt) const;

  // Returns true when |pt| belongs to the country identified by |id|.
  virtual bool BelongsToRegion(m2::PointD const & pt, size_t id) const = 0;

  // List of all known countries.
  std::vector<CountryDef> m_countries;
};

// *NOTE* This class is thread-safe.
class CountryInfoGetter : public CountryInfoGetterBase
{
public:
  // Returns vector of countries file names without extension for
  // countries belonging to |rect|. When |rough| is equal to true, the
  // method is much faster but the result is less precise.
  std::vector<CountryId> GetRegionsCountryIdByRect(m2::RectD const & rect, bool rough) const;

  // Returns a list of country ids by a |pt| in mercator.
  // |closestCoutryIds| is filled with country ids of mwms that cover |pt| or are close to it
  // with the exception of World.mwm and custom user-provided mwms.
  // The result may be empty, for example if |pt| is somewhere in an ocean.
  void GetRegionsCountryId(m2::PointD const & pt, CountriesVec & closestCoutryIds,
                           double lookupRadiusM = 30000.0 /* 30 km */) const;

  // Fills info for the region |pt| belongs to.
  void GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const;

  // Fills info for the country by id.
  void GetRegionInfo(CountryId const & countryId, CountryInfo & info) const;

  // Fills limit rects of the USA:
  // 0 - continental part
  // 1 - Alaska
  // 2 - Hawaii
  void CalcUSALimitRect(m2::RectD rects[3]) const;

  // Calculates the limit rect for all countries whose names start with |prefix|.
  m2::RectD CalcLimitRect(std::string const & prefix) const;

  // Returns the limit rect for |countryId| (non-expandable node).
  // Returns the bounding box in mercator coordinates if |countryId| is a country id of
  // a non-expandable node and zero rect otherwise.
  m2::RectD GetLimitRectForLeaf(CountryId const & leafCountryId) const;

  // Returns identifiers for all regions matching to |affiliation|.
  virtual void GetMatchedRegions(std::string const & affiliation, RegionIdVec & regions) const;

  // Clears the regions cache.
  void ClearCaches() const { ClearCachesImpl(); }

  void SetAffiliations(Affiliations const * affiliations);

protected:
  CountryInfoGetter() = default;

  // Invokes |toDo| on each country whose name starts with |prefix|.
  template <typename ToDo>
  void ForEachCountry(std::string const & prefix, ToDo && toDo) const;

  // Clears regions cache.
  virtual void ClearCachesImpl() const = 0;

  // Returns true when |rect| intersects a country identified by |id|.
  virtual bool IsIntersectedByRegion(m2::RectD const & rect, size_t id) const = 0;

  // Returns true when the distance from |pt| to country identified by |id| is less than |distance|.
  virtual bool IsCloseEnough(size_t id, m2::PointD const & pt, double distance) const = 0;

  // @TODO(bykoianko): consider getting rid of m_countryIndex.
  // Maps all leaf country id (file names) to their indices in m_countries.
  std::unordered_map<CountryId, RegionId> m_countryIndex;

  Affiliations const * m_affiliations = nullptr;

  // Maps country file name without extension to a country info.
  std::map<std::string, CountryInfo> m_idToInfo;
};

// This class reads info about countries from polygons file and
// countries file and caches it.
class CountryInfoReader : public CountryInfoGetter
{
public:
  /// \returns CountryInfoReader/CountryInfoGetter based on countries.txt and packed_polygons.bin.
  static std::unique_ptr<CountryInfoReader> CreateCountryInfoReader(Platform const & platform);
  static std::unique_ptr<CountryInfoGetter> CreateCountryInfoGetter(Platform const & platform);

  // Loads all regions for country number |id| from |m_reader|.
  void LoadRegionsFromDisk(size_t id, std::vector<m2::RegionD> & regions) const;

protected:
  CountryInfoReader(ModelReaderPtr polyR, ModelReaderPtr countryR);

  // CountryInfoGetter overrides:
  void ClearCachesImpl() const override;
  bool BelongsToRegion(m2::PointD const & pt, size_t id) const override;
  bool IsIntersectedByRegion(m2::RectD const & rect, size_t id) const override;
  bool IsCloseEnough(size_t id, m2::PointD const & pt, double distance) const override;

  template <typename Fn>
  std::invoke_result_t<Fn, std::vector<m2::RegionD>> WithRegion(size_t id, Fn && fn) const;

  FilesContainerR m_reader;
  mutable base::Cache<uint32_t, std::vector<m2::RegionD>> m_cache;
  mutable std::mutex m_cacheMutex;
};

// This class allows users to get info about very simply rectangular
// countries, whose info can be described by CountryDef instances.
// It's needed for testing purposes only.
class CountryInfoGetterForTesting : public CountryInfoGetter
{
public:
  CountryInfoGetterForTesting() = default;
  CountryInfoGetterForTesting(std::vector<CountryDef> const & countries);

  void AddCountry(CountryDef const & country);

  // CountryInfoGetter overrides:
  void GetMatchedRegions(std::string const & affiliation, RegionIdVec & regions) const override;

protected:
  // CountryInfoGetter overrides:
  void ClearCachesImpl() const override;
  bool BelongsToRegion(m2::PointD const & pt, size_t id) const override;
  bool IsIntersectedByRegion(m2::RectD const & rect, size_t id) const override;
  bool IsCloseEnough(size_t id, m2::PointD const & pt, double distance) const override;
};
}  // namespace storage
