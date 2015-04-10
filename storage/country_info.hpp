#pragma once

#include "storage/country_decl.hpp"

#include "geometry/region2d.hpp"

#include "coding/file_container.hpp"

#include "base/cache.hpp"


namespace storage
{
  class CountryInfoGetter
  {
    FilesContainerR m_reader;

    vector<CountryDef> m_countries;

    template <class ToDo> void ForEachCountry(string const & prefix, ToDo toDo) const;

    /// ID - is a country file name without an extension.
    map<string, CountryInfo> m_id2info;

    mutable my::Cache<uint32_t, vector<m2::RegionD> > m_cache;

    vector<m2::RegionD> const & GetRegions(size_t id) const;

    template <class ToDo>
    void ForEachCountry(m2::PointD const & pt, ToDo & toDo) const;

    class GetByPoint
    {
      CountryInfoGetter const & m_info;
      m2::PointD const & m_pt;

    public:
      size_t m_res;

      GetByPoint(CountryInfoGetter const & info, m2::PointD const & pt)
        : m_info(info), m_pt(pt), m_res(-1)
      {
      }

      /// @param[in] id Index in m_countries.
      /// @return false If point is in country.
      bool operator() (size_t id);
    };

  public:
    CountryInfoGetter(ModelReaderPtr polyR, ModelReaderPtr countryR);

    string GetRegionFile(m2::PointD const & pt) const;

    /// @param[in] pt Point inside country.
    void GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const;
    /// @param[in] id Country file name without an extension.
    void GetRegionInfo(string const & id, CountryInfo & info) const;

    /// Return limit rects of USA:\n
    /// 0 - continental part;\n
    /// 1 - Alaska;\n
    /// 2 - Hawaii;\n
    void CalcUSALimitRect(m2::RectD rects[3]) const;

    m2::RectD CalcLimitRect(string const & prefix) const;

    /// @name Used for checking that objects (by point) are belong to regions.
    //@{
    /// ID of region (index in m_countries array).
    typedef size_t IDType;
    typedef vector<IDType> IDSet;

    /// @param[in]  enName  English name to match (should be in lower case).
    /// Search is case-insensitive.
    void GetMatchedRegions(string const & enName, IDSet & regions) const;
    bool IsBelongToRegion(m2::PointD const & pt, IDSet const & regions) const;
    bool IsBelongToRegion(string const & fileName, IDSet const & regions) const;
    //@}

    /// m_cache is mutable.
    void ClearCaches() const;
  };
}
