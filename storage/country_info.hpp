#pragma once

#include "country_decl.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/file_container.hpp"

#include "../base/cache.hpp"


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
        : m_info(info), m_pt(pt), m_res(-1) {}
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
  };
}
