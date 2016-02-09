#pragma once

#include "storage/country_decl.hpp"

#include "geometry/region2d.hpp"

#include "coding/file_container.hpp"

#include "base/cache.hpp"

#include "std/mutex.hpp"
#include "std/unordered_map.hpp"

namespace storage
{
// This class allows users to get information about country by point
// or by name.
//
// *NOTE* This class is thread-safe.
class CountryInfoGetter
{
public:
  // Identifier of a region (index in m_countries array).
  using IdType = size_t;
  using IdSet = vector<IdType>;

  CountryInfoGetter(bool isSingleMwm) : m_isSingleMwm(isSingleMwm) {}
  virtual ~CountryInfoGetter() = default;

  // Returns country file name without an extension for a country |pt|
  // belongs to. If there are no such country, returns an empty
  // string.
  TCountryId GetRegionCountryId(m2::PointD const & pt) const;

  // Returns a list of country ids by a |pt| in mercator.
  // |closestCoutryIds| is filled with country ids of mwm which covers |pt| or close to it.
  // |closestCoutryIds| is not filled with country world.mwm country id and with custom mwm.
  // If |pt| is covered by a sea or a ocean closestCoutryIds may be left empty.
  void GetRegionsCountryId(m2::PointD const & pt, TCountriesVec & closestCoutryIds);

  // Returns info for a region |pt| belongs to.
  void GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const;

  // Returns info for a country by file name without an extension.
  void GetRegionInfo(string const & id, CountryInfo & info) const;

  // Return limit rects of USA:
  // 0 - continental part
  // 1 - Alaska
  // 2 - Hawaii
  void CalcUSALimitRect(m2::RectD rects[3]) const;

  // Calculates limit rect for all countries whose name starts with
  // |prefix|.
  m2::RectD CalcLimitRect(string const & prefix) const;
  // Calculates limit rect for |countryId| (non-expandable node).
  // Returns bound box in mercator coordinates if |countryId| is a country id of non-expandable node
  // and zero rect otherwise.
  m2::RectD CalcLimitRectForLeaf(TCountryId leafCountryId) const;

  // Returns identifiers for all regions matching to |enNamePrefix|.
  void GetMatchedRegions(string const & enNamePrefix, IdSet & regions) const;

  // Returns true when |pt| belongs to at least one of the specified
  // |regions|.
  bool IsBelongToRegions(m2::PointD const & pt, IdSet const & regions) const;

  // Returns true if there're at least one region with name equals to
  // |fileName|.
  bool IsBelongToRegions(string const & fileName, IdSet const & regions) const;

  // Clears regions cache.
  inline void ClearCaches() const { ClearCachesImpl(); }

protected:
  CountryInfoGetter() = default;

  // Returns identifier of a first country containing |pt|.
  IdType FindFirstCountry(m2::PointD const & pt) const;

  // Invokes |toDo| on each country whose name starts with |prefix|.
  template <typename ToDo>
  void ForEachCountry(string const & prefix, ToDo && toDo) const;

  // Clears regions cache.
  virtual void ClearCachesImpl() const = 0;

  // Returns true when |pt| belongs to a country identified by |id|.
  virtual bool IsBelongToRegionImpl(size_t id, m2::PointD const & pt) const = 0;

  // @TODO(bykoianko) Probably it's possible to redesign the class to get rid of m_countryIndex.
  // The possibility should be considered.
  // List of all known countries.
  vector<CountryDef> m_countries;
  // Maps all leaf country id (file names) to their index in m_countries.
  unordered_map<TCountryId, IdType> m_countryIndex;

  // Maps country file name without an extension to a country info.
  map<string, CountryInfo> m_id2info;

  // m_isSingleMwm == true if the system is currently working with single (small) mwms
  // and false otherwise.
  // @TODO(bykoianko) Init m_isSingleMwm correctly.
  bool m_isSingleMwm;
};

// This class reads info about countries from polygons file and
// countries file and caches it.
class CountryInfoReader : public CountryInfoGetter
{
public:
  CountryInfoReader(ModelReaderPtr polyR, ModelReaderPtr countryR);

protected:
  // CountryInfoGetter overrides:
  void ClearCachesImpl() const override;
  bool IsBelongToRegionImpl(size_t id, m2::PointD const & pt) const override;

  FilesContainerR m_reader;
  mutable my::Cache<uint32_t, vector<m2::RegionD>> m_cache;
  mutable mutex m_cacheMutex;
};

// This class allows users to get info about very simply rectangular
// countries, whose info can be described by CountryDef instances.
// It's needed for testing purposes only.
class CountryInfoGetterForTesting : public CountryInfoGetter
{
public:
  CountryInfoGetterForTesting() = default;
  CountryInfoGetterForTesting(vector<CountryDef> const & countries);

  void AddCountry(CountryDef const & country);

protected:
  // CountryInfoGetter overrides:
  void ClearCachesImpl() const override;
  bool IsBelongToRegionImpl(size_t id, m2::PointD const & pt) const override;
};
}  // namespace storage
