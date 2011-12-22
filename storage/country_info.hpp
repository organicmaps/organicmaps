#pragma once
#include "country_polygon.hpp"

#include "../geometry/region2d.hpp"

#include "../coding/file_container.hpp"

#include "../base/cache.hpp"


namespace storage
{
  class CountryInfoGetter
  {
    FilesContainerR m_reader;

    vector<CountryDef> m_countries;
    map<string, string> m_id2name;

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
    string GetRegionName(m2::PointD const & pt) const;
    string GetRegionName(string const & id) const;
  };
}
