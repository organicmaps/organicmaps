#pragma once

#include "storage/country_decl.hpp"

#include "geometry/region2d.hpp"

#include "coding/file_container.hpp"

#include "base/cache.hpp"

#include "std/mutex.hpp"

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

  CountryInfoGetter(ModelReaderPtr polyR, ModelReaderPtr countryR);

  // Returns country file name without an extension for a country |pt|
  // belongs to. If there are no such country, returns an empty
  // string.
  string GetRegionFile(m2::PointD const & pt) const;

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

  // Returns identifiers for all regions matching to |enNamePrefix|.
  void GetMatchedRegions(string const & enNamePrefix, IdSet & regions) const;

  // Returns true when |pt| belongs to at least one of the specified
  // |regions|.
  bool IsBelongToRegions(m2::PointD const & pt, IdSet const & regions) const;

  // Returns true if there're at least one region with name equals to
  // |fileName|.
  bool IsBelongToRegions(string const & fileName, IdSet const & regions) const;

  // Clears regions cache.
  void ClearCaches() const;

private:
  // Returns true when |pt| belongs to a country identified by |id|.
  bool IsBelongToRegion(size_t id, m2::PointD const & pt) const;

  // Returns identifier of a first country containing |pt|.
  IdType FindFirstCountry(m2::PointD const & pt) const;

  // Invokes |toDo| on each country whose name starts with |prefix|.
  template <typename ToDo>
  void ForEachCountry(string const & prefix, ToDo && toDo) const;

  vector<CountryDef> m_countries;

  // Maps country file name without an extension to a country info.
  map<string, CountryInfo> m_id2info;

  // Only cache and reader can be modified from different threads, so
  // they're guarded by m_cacheMutex.
  FilesContainerR m_reader;
  mutable my::Cache<uint32_t, vector<m2::RegionD>> m_cache;
  mutable mutex m_cacheMutex;
};
}  // namespace storage
