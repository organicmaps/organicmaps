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

    void GetRegionInfo(m2::PointD const & pt, CountryInfo & info) const;
    void GetRegionInfo(string const & id, CountryInfo & info) const;
  };
}
